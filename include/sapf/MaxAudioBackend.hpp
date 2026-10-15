#pragma once

#include <memory>
#include <vector>

#include "sapf/AudioBackend.hpp"

class MaxAudioBackend : public AudioBackend {
public:
	MaxAudioBackend();
	~MaxAudioBackend() override;

	void play(Thread& th, V& v) override;
	void record(Thread& th, V& v, Arg filename) override;
	void stopAll() override;
	void stopFinished() override;

	void render(float** outputs, int numChannels, int numFrames);

	// The next play() first stops all players, so a new evaluation replaces the sound of the last.
	void setStopOnNextPlay(bool on);

private:
	struct Player;
	void removeFinishedLocked();

	std::vector<std::unique_ptr<Player>> players_;
	std::vector<float> scratch_;
	bool stopOnNextPlay_ = false;
};

MaxAudioBackend& InstallMaxMSPBackend();
void MaxMSPProcessAudio(float** outputs, int numChannels, int numFrames);

// The host's input for the block it is about to render, read by `adc`. Call on the audio thread
// before MaxMSPProcessAudio. Channels past numChannels read silence.
void MaxMSPSetInputBuffers(const float* const* inputs, int numChannels, int numFrames);

// Defines `adc` and `adcn`. Call after SapfEngine::initialize().
void RegisterMaxAudioPrimitives();
