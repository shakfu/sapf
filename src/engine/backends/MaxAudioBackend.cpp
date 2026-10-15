/*
 * Host-driven audio for any embedding (Max, Pd, VST3, CLAP): call InstallMaxMSPBackend()
 * during setup, then MaxMSPProcessAudio(outputs, numChannels, numFrames) from the host's
 * audio callback. render() never blocks or throws. See examples/host_audio.cpp.
 */

#include "sapf/MaxAudioBackend.hpp"

#include <algorithm>
#include <atomic>
#include <climits>
#include <cstring>
#include <mutex>

namespace {

const int kMaxChannels = 32;
std::mutex gMaxBackendMutex;

const int kMaxInputs = 16;
const int64_t kInputFrames = 1 << 14;  // a power of two; 0.34 s at 48 kHz

// Host input, written by MaxMSPSetInputBuffers. Frame f of channel c is data[c][f % kInputFrames].
struct InputFifo {
	float data[kMaxInputs][kInputFrames] = {};
	std::atomic<int64_t> written{0};
	std::atomic<int> hostFrames{0};  // the last host block
};
InputFifo gInput;

// One input channel as a signal. A render pulls a host block, in sapf blocks, so a stream starts a
// host block and a sapf block behind the input written. A pull past the input written gets zeros for
// the missing frames and stays that much further behind; a stream that falls out of the FIFO restarts.
class AdcGen : public Gen
{
	int mChannel;
	int64_t mPos = INT64_MIN;  // the next frame to read; INT64_MIN before the first pull

public:
	AdcGen(Thread& th, int channel)
		: Gen(th, itemTypeZ, false)
		, mChannel(channel)
	{
	}

	const char* TypeName() const override { return "AdcGen"; }

	void pull(Thread& th) override
	{
		int n = mBlockSize;
		Z* out = mOut->fulfillz(n);
		int64_t w = gInput.written.load(std::memory_order_acquire);
		if (mPos == INT64_MIN || mPos < w - kInputFrames / 2) mPos = w - gInput.hostFrames.load(std::memory_order_relaxed) - n;
		int avail = (int)std::clamp<int64_t>(w - mPos, 0, n);
		const float* in = gInput.data[mChannel];
		for (int i = 0; i < avail; ++i) {
			int64_t f = mPos + i;
			out[i] = f < 0 ? 0. : in[f & (kInputFrames - 1)];
		}
		std::fill(out + avail, out + n, 0.);
		mPos += avail;
		produce(0);
	}
};

} // namespace

struct MaxAudioBackend::Player {
	Player(const Thread& parentThread, int channels)
		: th(parentThread)
		, numChannels(channels)
		, in(channels)
	{
	}

	Thread th;
	int numChannels;
	std::vector<ZIn> in;
	bool done = false;
};

MaxAudioBackend::MaxAudioBackend() = default;
MaxAudioBackend::~MaxAudioBackend() = default;

void MaxAudioBackend::play(Thread& th, V& v)
{
	if (!v.isList()) wrongType("play : s", "List", v);

	std::unique_ptr<Player> player;

	if (v.isZList()) {
		player = std::make_unique<Player>(th, 1);
		player->in[0].set(v);
	} else {
		if (!v.isFinite()) indefiniteOp("play : s", "");
		P<List> s = (List*)v.o();
		s = s->pack(th, kMaxChannels);
		if (!s()) {
			post("Too many channels. Max is %d.\n", kMaxChannels);
			return;
		}
		Array* a = s->mArray();
		int asize = (int)a->size();
		player = std::make_unique<Player>(th, asize);
		for (int i = 0; i < asize; ++i) {
			player->in[i].set(a->at(i));
		}
		s = nullptr;
		a = nullptr;
	}
	v.o = nullptr;

	Locker lock(&gMaxBackendMutex);
	if (stopOnNextPlay_) {
		players_.clear();
		stopOnNextPlay_ = false;
	}
	players_.push_back(std::move(player));
}

void MaxAudioBackend::setStopOnNextPlay(bool on)
{
	Locker lock(&gMaxBackendMutex);
	stopOnNextPlay_ = on;
}

void MaxAudioBackend::record(Thread& th, V& v, Arg filename)
{
	(void)th;
	(void)v;
	(void)filename;
	post("record is not supported in the Max/MSP backend.\n");
	throw errFailed;
}

void MaxAudioBackend::stopAll()
{
	Locker lock(&gMaxBackendMutex);
	players_.clear();
}

void MaxAudioBackend::stopFinished()
{
	Locker lock(&gMaxBackendMutex);
	removeFinishedLocked();
}

void MaxAudioBackend::render(float** outputs, int numChannels, int numFrames)
{
	if (numChannels <= 0 || numFrames <= 0) {
		return;
	}

	for (int c = 0; c < numChannels; ++c) {
		std::memset(outputs[c], 0, static_cast<size_t>(numFrames) * sizeof(float));
	}
	// An interrupt is for the evaluation thread; the players keep sounding.
	struct IgnoreInterrupt {
		bool was = gIgnoreInterrupt;
		IgnoreInterrupt() { gIgnoreInterrupt = true; }
		~IgnoreInterrupt() { gIgnoreInterrupt = was; }
	} ignoreInterrupt;

	// Never block the host's audio thread: output silence while play()/stopAll() hold the lock.
	std::unique_lock<std::mutex> lock(gMaxBackendMutex, std::try_to_lock);
	if (!lock.owns_lock()) {
		return;
	}

	scratch_.resize(numFrames);

	auto it = players_.begin();
	while (it != players_.end()) {
		Player& player = *(*it);
		bool done = true;
		int channels = std::min(player.numChannels, numChannels);

		// An exception must not unwind into the host's callback; drop the player instead.
		try {
			for (int ch = 0; ch < channels; ++ch) {
				int frames = numFrames;
				bool channelDone = player.in[ch].fill(player.th, frames, scratch_.data(), 1);
				for (int i = 0; i < frames; ++i) {
					outputs[ch][i] += scratch_[i];
				}
				done = done && channelDone;
			}
		} catch (int err) {
			if (err <= -1000 && err > -1000 - kNumErrors) {
				post("\nerror: %s\n", errString[-1000 - err]);
			} else {
				post("\nerror: %d\n", err);
			}
			post("exception in real time. stopping player.\n");
			done = true;
		} catch (...) {
			post("\nexception in real time. stopping player.\n");
			done = true;
		}

		player.done = done;

		if (player.done) {
			it = players_.erase(it);
		} else {
			++it;
		}
	}
}

void MaxAudioBackend::removeFinishedLocked()
{
	auto it = players_.begin();
	while (it != players_.end()) {
		if ((*it)->done) {
			it = players_.erase(it);
		} else {
			++it;
		}
	}
}

MaxAudioBackend& InstallMaxMSPBackend()
{
	auto backend = std::make_unique<MaxAudioBackend>();
	MaxAudioBackend* raw = backend.get();
	SetAudioBackend(std::move(backend));
	return *raw;
}

void MaxMSPProcessAudio(float** outputs, int numChannels, int numFrames)
{
	if (!HasAudioBackend()) {
		return;
	}
	AudioBackend* base = nullptr;
	try {
		base = &GetAudioBackend();
	} catch (...) {
		return;
	}
	auto* backend = dynamic_cast<MaxAudioBackend*>(base);
	if (!backend) {
		return;
	}
	backend->render(outputs, numChannels, numFrames);
}

void MaxMSPSetInputBuffers(const float* const* inputs, int numChannels, int numFrames)
{
	int64_t w = gInput.written.load(std::memory_order_relaxed);
	for (int done = 0; done < numFrames;) {
		int64_t at = (w + done) & (kInputFrames - 1);
		int n = (int)std::min<int64_t>(numFrames - done, kInputFrames - at);
		for (int c = 0; c < kMaxInputs; ++c) {
			float* dst = &gInput.data[c][at];
			if (c < numChannels && inputs[c]) {
				std::memcpy(dst, inputs[c] + done, n * sizeof(float));
			} else {
				std::memset(dst, 0, n * sizeof(float));
			}
		}
		done += n;
	}
	gInput.hostFrames.store(numFrames, std::memory_order_relaxed);
	gInput.written.store(w + numFrames, std::memory_order_release);
}

static int popInput(Thread& th, const char* msg, int64_t lo)
{
	int64_t i = th.popInt(msg);
	if (i < lo || i >= kMaxInputs + lo) {
		post("%s must be %d to %d\n", msg, (int)lo, kMaxInputs - 1 + (int)lo);
		throw errOutOfRange;
	}
	return (int)i;
}

static void adc_(Thread& th, Prim* prim)
{
	int channel = popInput(th, "adc : channel", 0);
	th.push(new List(new AdcGen(th, channel)));
}

static void adcn_(Thread& th, Prim* prim)
{
	int n = popInput(th, "adcn : numChannels", 1);
	P<List> s = new List(itemTypeV, n);
	for (int c = 0; c < n; ++c) {
		s->add(new List(new AdcGen(th, c)));
	}
	th.push(s);
}

void RegisterMaxAudioPrimitives()
{
	vm.addBifHelp("\n*** host audio input ***");
	vm.defmcx("adc", 1, adc_, "(channel --> signal) the host's audio input channel, from 0.");
	vm.def("adcn", 1, 1, adcn_, "(numChannels --> signals) the host's audio inputs 0 to numChannels - 1.");
}
