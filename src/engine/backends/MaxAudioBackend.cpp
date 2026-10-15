/*
 * Host-driven audio for any embedding (Max, Pd, VST3, CLAP): call InstallMaxMSPBackend()
 * during setup, then MaxMSPProcessAudio(outputs, numChannels, numFrames) from the host's
 * audio callback. render() never blocks or throws. See examples/host_audio.cpp.
 */

#include "sapf/MaxAudioBackend.hpp"

#include <algorithm>
#include <cstring>
#include <mutex>

namespace {

const int kMaxChannels = 32;
std::mutex gMaxBackendMutex;

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
