# Changelog

All notable changes to SAPF (Sound As Pure Form) will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added

- **ARCHITECTURE.md** - Comprehensive documentation of the VM execution model, reference counting system, and threading guarantees
- **REVIEW.md** - Code review with analysis of architecture, modularity, and recommendations
- **Platform abstraction layer** (`include/sapf/platform/Platform.hpp`) - Cross-platform interface for async execution and REPL loops
  - macOS implementation using Grand Central Dispatch and CFRunLoop
  - Linux implementation using std::thread and condition variables

### Changed

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

- Memory management in delay UGens now uses RAII, preventing potential leaks on early returns or exceptions

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
