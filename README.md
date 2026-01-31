# SAPF - Sound As Pure Form

A concatenative functional language for exploring sound synthesis and transformation.

## Overview

SAPF is an interpreter for a language designed to create and transform sound. The language is:

- **Mostly functional** with immutable data structures
- **Stack-based** with postfix notation (similar to Forth)
- **Lazy** with support for potentially infinite sequences
- **APL-inspired** with pervasive automatic mapping, scanning, and reduction

Short programs can achieve results out of proportion to their size. Because nearly all data types are immutable, the language can run multiple threads without deadlock or corruption.

## Inspirations

- **APL** - Automatic mapping over arrays, operations at depth
- **Joy/Forth** - Concatenative programming, stack-based VM
- **Haskell** - Lazy evaluation, functional purity
- **Piccola** - Prototype-based objects
- **Nyquist/SuperCollider** - Sound synthesis paradigms

## Quick Example

```
;; Play a sine wave at 800 Hz
800 0 sinosc .3 * play

;; The classic "analog bubbles" from SuperCollider:
.4 0 lfsaw 2 * [8 7.23] 0 lfsaw .25 * 5/3 + + ohz 0 sinosc .04 * .2 0 4 combn play

;; Type 'stop' to stop playback
```

## Building

### Prerequisites

- CMake 3.16+
- C++17 compatible compiler
- macOS or Linux

### Build Commands

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
make test
```

### Installation

```bash
# Copy binary to your path
cp build/sapf ~/bin/

# On macOS, remove quarantine if needed
xattr -r -d com.apple.quarantine ~/bin/sapf
```

## Environment Variables

```bash
export SAPF_HISTORY="$HOME/sapf-files/sapf-history.txt"
export SAPF_LOG="$HOME/sapf-files/sapf-log.txt"
export SAPF_PRELUDE="$HOME/sapf-files/sapf-prelude.txt"
export SAPF_EXAMPLES="$HOME/sapf-files/sapf-examples.txt"
export SAPF_README="$HOME/sapf-files/README.txt"
export SAPF_RECORDINGS="$HOME/sapf-files/recordings"
export SAPF_SPECTROGRAMS="$HOME/sapf-files/spectrograms"
```

## Command Line Options

```
sapf [-r sample-rate] [-p prelude-file]
sapf [-h]

Options:
  -r sample-rate    Set session sample rate (default: 96000 Hz)
  -p prelude-file   Load code before entering REPL
  -h                Print help
```

## Core Types

| Type | Description |
|------|-------------|
| **Real** | 64-bit double precision floating point |
| **String** | Character strings for naming |
| **List** | Ordered, lazy, potentially infinite sequences |
| **Form** | Dictionary with inheritance (prototype chain) |
| **Function** | First-class functions with closures |
| **Ref** | Mutable container (only mutable type) |

## Syntax Highlights

### Numbers
```
1  2.3  .5  7.          ;; standard notation
3.4e-3  1.7e4           ;; scientific
2pi  .5pi               ;; pi suffix
4k  1.5k                ;; kilo (1000x)
125m  20u               ;; milli, micro
5/4  9/7                ;; inline fractions
```

### Lists and Signals
```
[1 2 3]                 ;; value list (stream)
#[1 2 3]                ;; numeric list (signal)
[1 2 + 3 4 *]  -->  [3 12]
```

### Forms (Objects)
```
{ :a 1 :b 2 } = x       ;; create form
{ x :c 3 } = y          ;; inherit from x
y ,a                    ;; access: 1
```

### Functions
```
\a b [a b + a b *] = blub
3 4 blub  -->  7 12
```

### Auto-mapping
```
0 4 to        -->  [0 1 2 3 4]
[0 2] 4 to    -->  [[0 1 2 3 4] [2 3 4]]
```

### Each Operator
```
[[1 2 3] [4 5 6]] @ reverse   -->  [[3 2 1] [6 5 4]]
[1 2] @ [10 20] +             -->  [[11 21] [12 22]]
```

### Reducing and Scanning
```
[1 2 3 4] +/   -->  10           ;; reduce (sum)
[1 2 3 4] +\   -->  [1 3 6 10]   ;; scan (accumulate)
[1 2 3 4] */   -->  24           ;; product
```

## Documentation

- **[README.txt](README.txt)** - Full language reference and tutorial
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - VM execution model and internals
- **[CONTRIBUTING.md](CONTRIBUTING.md)** - Contribution guidelines
- **[TODO.md](TODO.md)** - Planned features and improvements

## Getting Help

```
helpall              ;; list all functions
`someword help       ;; help for specific function
```

## Project Structure

```
include/
  Object.hpp         ;; Core object system (orchestrates modular headers)
  Forward.hpp        ;; Forward declarations and type aliases
  Value.hpp          ;; V (tagged value) class
  ObjectBase.hpp     ;; Object base class
  String.hpp         ;; Interned string class
  ObjectInlines.hpp  ;; Inline implementations
  sapf/platform/     ;; Platform abstraction layer

src/
  engine/            ;; Core interpreter and VM
  cli/               ;; Command-line interface
  audio/             ;; Audio I/O (RtAudio)
  midi/              ;; MIDI I/O (RtMidi)

tests/               ;; Test suite
```

## License

GNU General Public License v3.0 - See [LICENSE](LICENSE) for details.

Copyright (C) 2019 James McCartney
