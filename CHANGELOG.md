# Changelog

All notable changes to SAPF (Sound As Pure Form) will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added

- **Cross-platform audio recording** via libsndfile for Linux and Windows
  - `AlsaAudioBackend` - Recording support using libsndfile (Linux)
  - `RtAudioBackend` - Recording support using libsndfile (Linux/Windows)
  - Records to WAV format with automatic file opening on completion
  - macOS continues to use native ExtAudioFile API
- **Cross-platform MIDI testing** - MIDI smoke tests now run on all platforms
  - Updated test patterns to work with both CoreMidi and RtMidi backends
  - `midi_list`, `midi_start_stop`, `midi_debug` tests enabled on Linux/Windows
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

- **CI/CD workflow** - Full test coverage on all platforms
  - Linux: All 264 tests including MIDI (was excluding `^midi_` pattern)
  - Windows: All 264 tests including MIDI, libsndfile enabled via vcpkg
  - macOS: All 264 tests (unchanged)
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
