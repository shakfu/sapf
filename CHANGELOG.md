# Changelog

All notable changes to SAPF (Sound As Pure Form) will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added

- **`adc` and `adcn`** (`sapf/MaxAudioBackend.hpp`) - A host's audio input as signals, from sapf-max. `MaxMSPSetInputBuffers` copies each host block into a FIFO, and a stream starts one host block plus one sapf block behind the input written. sapf-max reads the current host buffer on every pull instead; a graph pulls in sapf blocks, not host blocks, so that repeats or skips input whenever the two sizes differ. A pull past the input written gets zeros and stays behind by that much. `RegisterMaxAudioPrimitives` defines both. `MaxMSPSetInputLead` adds a host's render-ahead to the start offset: an underflowing pull copies the frames it has and pads the rest with zeros, so without it a render thread ahead of its input leaves gaps inside the input.
- **Interrupt** (`gInterrupt`, `VM.hpp`) - A host sets it to stop a running evaluation, which then throws `errInterrupted` at its next opcode or list segment. `MaxAudioBackend::render` ignores it, so interrupting an evaluation leaves the sound playing.
- **`SapfEngineConfig::blockSize`** (`VM::setBlockSize`) - Frames a signal UGen computes per pull, default 512; a multiple of 128, since the control rate takes one frame per 128. A host can set its own block size so a render's cost does not arrive in 512-frame bursts. Noise UGens draw from their thread's generator as they pull, so a different block size gives them different random values; other output is unchanged.
- **`SapfEngineConfig::openFiles`** - Off stops sapf handing written sound files, spectrograms and the help files to a viewer through `system()`. An audio plugin needs it off: `system()` forks the whole host process. On by default, so the REPL is unchanged.
- **`SAPF_ENGINE_ONLY`** - Builds only the engine and FFTW, for an embedding host such as the SSP plugin. New options `SAPF_REPLXX` and `SAPF_USE_ALSA` turn off replxx and the ALSA backend; without replxx the engine exports `USE_REPLXX=0`, which changes `Thread`'s layout for every user of the headers. A host can supply its own libsndfile as target `SndFile::sndfile`.

- **CLAP plugin** (`examples/clap/`) - Stereo instrument with a code editor on macOS, Windows and Linux, and loopback UDP input; each datagram is evaluated and the output sent back. Eval Block (Ctrl/Cmd+Return) evaluates the selection, the current line, or the multi-line `( )`, `[ ]` or `{ }` group it is in; Eval All (Ctrl/Cmd+Shift+Return) evaluates the whole editor; Stop (Ctrl+.) stops; Clear empties editor and log after confirming; Wrap (on by default) wraps long lines in both panes; Replace (off by default) makes an evaluation's first `play` stop the sounds already playing, via `MaxAudioBackend::setStopOnNextPlay`. Editor text, Wrap, Replace and UDP settings persist in host state. One instance per process. `test_clap_host` drives the built plugin headlessly as ctest `example_clap_plugin`; `make editor` opens it in `sapf_dev_host`, a minimal pugl + RtAudio host. See `examples/clap/README.md`.

  The editor is Dear ImGui v1.93.0 in a pugl OpenGL view, both vendored under `thirdparty/`. pugl over GLFW because pugl embeds a child view in the host window and never pumps the host's event loop. Stop is Ctrl+. because AppKit consumes Cmd+. before any view sees it. `sapf-prelude.txt` is compiled in and loaded at startup, because a DAW does not inherit `SAPF_PRELUDE` from the shell; setting it still overrides.
- **`SapfSetPostHook`** (`sapf/Engine.hpp`) - Routes `post()` output to a callback instead of stdout, so embedding hosts can show it. `post()` also gains its missing `va_end`.
- **Host-driven audio example** (`examples/host_audio.cpp`) - Embeds sapf in a host that owns the audio callback (Pd external, VST3/CLAP plugin) and runs as ctest `example_host_audio`. `host_audio_module` builds the same file as a shared module, so a Linux build fails if non-PIC code reaches the engine.
- **Tests** - `repl_quit` and `repl_unwritable_log` cover the piped-stdin REPL path; `tests/unit/test_host_audio.cpp` covers `MaxAudioBackend` playback and render-time errors.
- **Cross-platform audio recording** via libsndfile for Linux and Windows
  - `AlsaAudioBackend` - Recording support using libsndfile (Linux)
  - `RtAudioBackend` - Recording support using libsndfile (Linux/Windows)
  - Records to WAV format with automatic file opening on completion
  - macOS continues to use native ExtAudioFile API
- **Cross-platform MIDI testing** - MIDI smoke tests now run on all platforms
  - Updated test patterns to work with both CoreMidi and RtMidi backends
  - `midi_list`, `midi_start_stop`, `midi_debug` tests enabled on Linux/Windows
  - On hosts without a MIDI subsystem (CI runners) they pass on the `RtMidi error` line and a clean exit; only macOS CI exercises a working MIDI backend
- **Cross-platform sound file I/O** using libsndfile for Linux and Windows
  - `sfread` - Streaming audio file reader with seeking support
  - `sfwrite` - Audio file writer (WAV format)
  - macOS continues to use native AudioToolbox/ExtAudioFile
  - Graceful fallback to stub implementations if libsndfile not installed
  - CMake auto-detection via pkg-config (`SAPF_USE_LIBSNDFILE` option)
- **Windows platform support** (`src/engine/platform/PlatformWindows.cpp`)
  - Async execution using std::thread
  - REPL event loop using std::mutex and condition variables
  - MSVC compiler compatibility for `__builtin_clzll` intrinsic
- **Unit test infrastructure** using Google Test framework
  - `tests/unit/test_refcount.cpp` - P<T> smart pointer and reference counting (16 tests)
  - `tests/unit/test_value.cpp` - V class operations, type checking, conversions (35 tests)
  - `tests/unit/test_symbol.cpp` - Symbol interning, thread safety, concurrent access (17 tests)
  - `tests/unit/test_hash.cpp` - Hash() and Hash64() functions, distribution tests (15 tests)
  - `tests/unit/test_parser.cpp` - Parser edge cases, number/string/array/lambda parsing (40 tests)
  - `tests/unit/test_vm.cpp` - VM opcodes, arithmetic, stack manipulation, conditionals (60 tests)
  - `tests/unit/test_array_list.cpp` - Array/List construction, indexing, operations (30 tests)
  - `tests/unit/test_errors.cpp` - Error handling, syntax errors, type errors (24 tests)
  - Total: 246 new unit tests alongside 18 existing integration tests (264 total)
- **Vendored GoogleTest v1.14.0** in `thirdparty/googletest/` for reproducible offline builds
- **ARCHITECTURE.md** - Comprehensive documentation of the VM execution model, reference counting system, and threading guarantees
- **REVIEW.md** - Code review with analysis of architecture, modularity, and recommendations
- **Platform abstraction layer** (`include/sapf/platform/Platform.hpp`) - Cross-platform interface for async execution and REPL loops
  - macOS implementation using Grand Central Dispatch and CFRunLoop
  - Linux implementation using std::thread and condition variables

### Changed

- **The REPL writes no files unless asked.** History stays in memory and there is no log. `SAPF_HISTORY` and `SAPF_LOG` name the files; set them to keep the old behaviour. The defaults were `~/sapf-history.txt` and `~/sapf-log.txt`, and the history went to the current directory when `HOME` was unset.
- **`setseed` reaches the threads a thread starts.** A player that `play` starts, and a `go` thread, seeded themselves from the clock, so `setseed` never fixed noise UGens' values or `ola` and `oltx` voices, which run on the player's thread: the same seeded code sounded different each time. A new thread now seeds itself from its parent's generator state and a count of the threads started since the parent was seeded. The parent's own numbers do not move, and an unseeded thread still starts from the clock, so code without `setseed` varies as before.

- **CI** runs on push and pull request to `main`, skips Markdown-only changes, and cancels superseded runs on the same ref. Artifact names end in the 7-character commit SHA. Linux installs X11 and OpenGL headers for the CLAP editor.
- **Position-independent code** is enabled for the engine and vendored libraries, so the static engine links into shared-library hosts.
- **`MaxAudioBackend::render` never blocks.** It uses `try_lock` and outputs one silent block if `play`/`stop` holds the lock, rather than waiting on the host's audio thread. A lock-free player queue would avoid the silent block; that is deferred.
- **CI/CD workflow** - Full test coverage on all platforms
  - Linux: All 284 tests including MIDI (was excluding `^midi_` pattern)
  - Windows: All 284 tests including MIDI, libsndfile enabled via vcpkg
  - macOS: All 284 tests (unchanged)
- **ARCHITECTURE.md** - Added comprehensive cross-platform architecture section
  - Platform abstraction layer documentation
  - Audio backend architecture with fallback diagrams
  - Recording support matrix by platform
  - DSP acceleration (Accelerate vs FFTW3)
  - MIDI backend architecture
  - Compiler compatibility (MSVC intrinsics)
  - Expanded appendix with backend file references
- **DelayUGens.cpp** - Replaced raw `malloc`/`free` allocations with `std::unique_ptr<Z[]>` for automatic memory management in all 13 delay unit generator classes:
  - DelayN, DelayL, DelayC
  - CombN, CombL, CombC
  - AllpassN, AllpassL, AllpassC
  - Pluck, PluckC
  - MultiTapN, MultiTapL

- **Object.hpp modularization** - Split the monolithic 1500+ line header into focused modules to break circular dependencies:
  - `Forward.hpp` - Forward declarations and type aliases (`O`, `Arg`, `Z`, `ItemType`)
  - `Value.hpp` - `V` (tagged value) class declaration and `UnaryOp`/`BinaryOp` base classes
  - `ObjectBase.hpp` - `Object` base class definition
  - `String.hpp` - `String` class (interned symbols)
  - `ObjectInlines.hpp` - Inline implementations requiring complete types (included last)

### Technical Details

The Object.hpp circular dependency (`V` <-> `Object`) was resolved using a standard C++ technique:
1. Forward declarations in `Forward.hpp` allow `P<Object>` in `V` without complete type
2. Method declarations in `Value.hpp` are separate from implementations
3. `ObjectInlines.hpp` provides inline implementations after all types are complete
4. Include order: `Forward.hpp` -> `Value.hpp` -> `ObjectBase.hpp` -> `String.hpp` -> [classes] -> `ObjectInlines.hpp`

### Fixed

- **`sinoscfb` started from uninitialized memory.** Its phase and feedback sample were never set, so its output depended on the heap: it differed between Linux x86 and ARM from the first sample and went NaN on ARM.
- **The FFTW inverse real FFT read an uninitialized Nyquist bin.** Callers pass n/2 bins, and the Accelerate branch zeroes Nyquist; the FFTW branch left it as whatever its buffer held. Wavetables render the same here, probably because the buffer came from fresh, zeroed pages.
- **32-bit list operations** - `Array::setSize` and `addSize` took `size_t`. `Gen::produce` passes a negative delta, which became 2^32 - k on 32-bit ARM and crashed every list operation.
- **Accelerate emulation off Apple** - `vDSP_ctozD`, `vDSP_ztocD` and `vDSP_rectD` counted interleaved strides in complex values; Accelerate counts doubles. `sgram`, written for Accelerate, read twice past its buffer. `vDSP_fft_zropD` ran a complex FFT of the full length on half as many inputs; it now packs and scales as Accelerate does. Both crashed every `sgram` on Linux. The wavetable fill now passes stride 2, so `saw`, `square`, `tri` and `pulse` render the same as before on Linux; on macOS they change from stride 1, unverified.

- **`unspell` read uninitialized memory and could crash.** It constructed the result `String`, which hashes its buffer immediately, before filling the buffer.
- **`dsf1` produced NaN when `coef` is exactly 1**, where both its normalization and its closed-form denominator are 0/0. It now uses the limit and, near the singularity, sums the series directly. A leftover debug `printf` that printed every sample to stdout is removed.
- **`sapf-bif-examples.txt`** used the nonexistent word `midihz`; it is `nnhz`.
- **REPL crash on any non-empty input when the log file cannot be opened.** The log path was built from `HOME` alone, so on Windows (no `HOME`) it became `(null)/sapf-log.txt`, `fopen` returned NULL, and the unchecked write aborted with `0xc0000409`; POSIX segfaulted the same way. Logging now falls back to `USERPROFILE` and is skipped when the file cannot be opened.
- **`MaxAudioBackend` let generator exceptions unwind into the host's audio callback.** The failing player is now dropped, as in `CoreAudioBackend`.
- **MIDI smoke tests** - The unquoted pattern `(gMIDIClient|RtMidi.initialized)` was split by CMake into an empty `EXPECTED_PATTERN`, so `midi_start_stop` passed on any output. Patterns are now quoted and matched against stdout and stderr, and accept the `RtMidi error` line on hosts without a MIDI subsystem (CI runners).
- **CoreAudioBackend.cpp** - Fixed compilation error with mutex type
  - Changed `pthread_mutex_t` to `std::mutex` for consistency with `Locker` class
  - Replaced `Locker lock(&gPlayerMutex)` with `std::lock_guard<std::mutex>`
- Memory management in delay UGens now uses RAII, preventing potential leaks on early returns or exceptions
- **Portable temporary directory paths** - Replaced hardcoded `/tmp` with `std::filesystem::temp_directory_path()` for Windows compatibility
  - `SoundFiles.cpp` - Recording path generation
  - `StreamOps.cpp` - Spectrogram output path

## [1.0.0] - Prior Version

Initial release of SAPF - a concatenative functional language for sound synthesis and transformation.

### Features

- Stack-based VM with opcode dispatch
- Lazy, potentially infinite sequences for audio and control
- Automatic mapping, scanning, and reduction (APL-inspired)
- Prototype-based object system with multiple inheritance
- Pull-based audio evaluation model
- Intrusive reference counting for memory management
- Real-time audio output via RtAudio
- MIDI input/output via RtMidi
- Interactive REPL with command history
