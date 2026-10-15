#pragma once

#include "VM.hpp"

struct SapfEngineConfig {
	double sampleRate = kDefaultSampleRate;
	// Frames a signal UGen computes per pull: a multiple of kDefaultControlBlockSize. A host can match its
	// own block, so a render's cost does not come in sapf-sized bursts.
	int blockSize = kDefaultZBlockSize;
	const char* preludeFile = nullptr;
	const char* logFile = nullptr;
	bool enableManta = true;
	// Off for a host whose process must not fork a viewer, such as an audio plugin
	bool openFiles = true;
};

class SapfEngine {
public:
	SapfEngine();

	void configure(const SapfEngineConfig& config);
	void initialize();
	void startMantaEventLoop() const;
	void loadPrelude(Thread& th) const;

	const char* versionString() const;
	const char* logFile() const;

private:
	void configureLogFile();
	void registerBuiltins();

	SapfEngineConfig config_;
	bool initialized_;
};

SapfEngine& GetSapfEngine();

// Receives each formatted post() message instead of stdout; nullptr restores stdout.
// Called from any thread that posts, including audio threads.
typedef void (*SapfPostHook)(const char* text, void* userData);
void SapfSetPostHook(SapfPostHook hook, void* userData);
const char* SapfGetVersionString();
