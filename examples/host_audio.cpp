// Embedding sapf in a host that owns the audio callback (Pd external, VST3/CLAP plugin).
//
// Runs one host lifecycle without an audio device:
//   setup     -> host_init()     (Pd: class new; CLAP: activate; VST3: setupProcessing)
//   control   -> host_eval()     (Pd: message inlet; plugin: main/UI thread)
//   audio     -> host_process()  (Pd: perform routine; CLAP: process; VST3: process)
// Exits 0 if a played sine reaches the output and `stop` silences it.

#include "sapf/Engine.hpp"
#include "sapf/MaxAudioBackend.hpp"
#include "VM.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {

const int kChannels = 2;
const int kBlockSize = 64;

void host_init(double sampleRate)
{
	// sampleRate is fixed here; it must match the host's rate.
	SapfEngineConfig config;
	config.sampleRate = sampleRate;
	config.enableManta = false;

	SapfEngine& engine = GetSapfEngine();
	engine.configure(config);
	engine.initialize();
	InstallMaxMSPBackend();
}

// Control thread only. Compiling and evaluating allocate and may block.
bool host_eval(Thread& th, const char* code)
{
	try {
		P<Fun> fun;
		if (!th.compile(code, fun, true)) return false;
		fun->run(th);
		return true;
	} catch (int err) {
		if (err <= -1000 && err > -1000 - kNumErrors) {
			fprintf(stderr, "error: %s\n", errString[-1000 - err]);
		} else {
			fprintf(stderr, "error: %d\n", err);
		}
	} catch (...) {
		fprintf(stderr, "error evaluating '%s'\n", code);
	}
	return false;
}

// Audio thread. Fills non-interleaved buffers; never blocks or throws.
void host_process(float** outputs, int numChannels, int numFrames)
{
	MaxMSPProcessAudio(outputs, numChannels, numFrames);
}

float peak_over_blocks(float** outputs, int numBlocks)
{
	float peak = 0.f;
	for (int b = 0; b < numBlocks; ++b) {
		host_process(outputs, kChannels, kBlockSize);
		for (int c = 0; c < kChannels; ++c) {
			for (int i = 0; i < kBlockSize; ++i) {
				peak = std::fmax(peak, std::fabs(outputs[c][i]));
			}
		}
	}
	return peak;
}

} // namespace

int main()
{
	host_init(48000.);
	Thread th;

	std::vector<float> left(kBlockSize), right(kBlockSize);
	float* outputs[kChannels] = { left.data(), right.data() };

	if (!host_eval(th, "440 0 sinosc .1 * play")) return 1;
	float playing = peak_over_blocks(outputs, 100);

	if (!host_eval(th, "stop")) return 1;
	float stopped = peak_over_blocks(outputs, 10);

	printf("peak while playing %.4f, after stop %.4f\n", playing, stopped);
	return (playing > 0.09f && playing < 0.11f && stopped == 0.f) ? 0 : 1;
}
