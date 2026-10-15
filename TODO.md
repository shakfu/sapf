# TODO

## Critical

## High

### Embedding (Pd external, VST3/CLAP plugin)

- [ ] Support multiple engine instances per process
  - Engine, audio backend and MIDI backend are process-wide singletons
  - Two instances calling `MaxMSPProcessAudio` pull the same players, so each gets alternate blocks

### Audio

- [ ] Catch exceptions in the RtAudio render callback (`RtAudioBackend.cpp:248`)
  - A generator error during playback unwinds into RtAudio; CoreAudio and host backends already catch

## Medium

### Features

- [ ] Design and implement plugin architecture: for third-party extensibility needed
- [ ] Add configuration file support - For complex setups

### Embedding

- [ ] Allow sample rate changes after `initialize()` (plugin activate, Pd DSP restart)
- [ ] Feed host MIDI events into `MidiRouter::handleIncomingMessage` (plugins receive MIDI in `process`, not via RtMidi)
- [ ] CLAP editor: verify on Windows and Linux hosts (CI only builds them)
- [ ] CLAP editor: syntax highlighting (ImGui text field is plain)
- [ ] CLAP: accept host MIDI note events (needs note ports and the `MidiRouter` item above)
- [ ] Add audio input to the host backend (needed for effect plugins)

### Documentation

- [ ] Add API documentation (Doxygen comments for public headers)
- [ ] Document naming conventions (camelCase methods, ALL_CAPS constants, etc.)
- [ ] Document Windows build status

### Build System

- [ ] Cache vcpkg in the Windows CI job (clone and build add ~4 min per run)

### Code Quality

- [ ] Add more unit tests: enable testing components in isolation

## Low

### Embedding

- [ ] Rename `MaxAudioBackend` / `InstallMaxMSPBackend` to host-neutral names (`HostAudioBackend`)
- [ ] Remove allocation and deallocation from the host backend's audio thread (`scratch_.resize`, player erase)

### Testing

- [ ] Make `tests/run_test.cmake` safe under `ctest -j` (all tests share `tests/temp_input.txt` in the source tree)

### Performance Optimization

- [ ] Integrate libxsimd for Linux SIMD optimization
  - The ahihi/sapf fork uses xsimd for cross-platform SIMD support
  - Would provide vectorized math operations on Linux similar to Apple's Accelerate framework
  - Reference: https://github.com/xtensor-stack/xsimd

### Platform Support

- [ ] Consider unified image format across platforms
- [ ] Plan C++20 migration path for cleaner compiler intrinsics
- [ ] Migrate to standard exceptions - Better debugging experience
- [ ] Consider native Windows audio backend (optional enhancement given RtAudio)

### Code Quality

- [ ] Add const-correctness audit for method parameters
- [ ] Consider extracting VM opcodes into separate compilation units
