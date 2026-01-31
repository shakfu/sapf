# SAPF - Sound As Pure Form

A concatenative functional language for exploring sound synthesis and transformation.

## Overview

SAPF is an interpreter for a language designed to create and transform sound. The language is:

- **Mostly functional** with immutable data structures
- **Stack-based** with postfix notation (similar to Forth)
- **Lazy** with support for potentially infinite sequences
- **APL-inspired** with pervasive automatic mapping, scanning, and reduction

Short programs can achieve results out of proportion to their size. Because nearly all data types are immutable, the language can run multiple threads without deadlock or corruption.

## Philosophy

> "What attracted me, then, to APL was a feeling that perhaps through APL one might begin to acquire some of the dimensions in programming that we revere in natural language - some of the pleasures of composition; of saying things elegantly; of being brief, poetic, artistic, that makes our natural languages so precious to us."
> -- Alan Perlis

APL and Forth are both widely derided for being write-only languages. Nevertheless, there has yet to be a language of such concise expressive power as APL or its descendants. APL is powerful not because of its bizarre symbols or syntax, but due to the way it automatically maps operations over arrays and allows iterations at depth within arrays. This means one almost never needs to write a loop or think about operations one-at-a-time. Instead one can think about operations on whole structures.

The Joy language introduced concatenative functional programming - a stack-based virtual machine where functions take an input stack and return an output stack. The natural syntax that results is postfix. Over a very long time I have come to feel that syntax gets in between me and the power in a language. Postfix is the least syntax possible.

### Why Concatenative Programming?

- Function composition is concatenation
- Pipelining values through functions is the most natural idiom
- Functions are applied from left to right instead of inside out
- Support for multiple return values comes for free
- No need for operator precedence
- Fewer delimiters required:
  - Parentheses are not needed to control operator precedence
  - Semicolons are not needed to separate statements
  - Commas are not needed to separate arguments

*(Note: SAPF is inspired by, but is not purely a concatenative language because it has lexical variables.)*

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
export SAPF_RECORDINGS="$HOME/sapf-files/recordings"
export SAPF_SPECTROGRAMS="$HOME/sapf-files/spectrograms"
```

| Variable | Description |
|----------|-------------|
| `SAPF_PRELUDE` | Code file loaded before entering the REPL |
| `SAPF_RECORDINGS` | Directory for output sound files |
| `SAPF_SPECTROGRAMS` | Directory for spectrogram images |
| `SAPF_HISTORY` | Command line history for recall at runtime |
| `SAPF_LOG` | Log of command line inputs |
| `SAPF_EXAMPLES` | Path to examples file |

## Command Line Options

```
sapf [-r sample-rate] [-p prelude-file] [-m] [-i] [-q] [file]

Options:
  -r sample-rate    Set session sample rate (default: 96000 Hz)
  -p prelude-file   Load code before entering REPL
  -m                Start Manta event loop
  -i                Interactive mode (enter REPL after running file)
  -q                Quiet mode (suppress banner)
  -h                Print help
```

## Core Types

> "It is better to have 100 functions operate on one data structure than 10 functions on 10 data structures." -- Alan Perlis

| Type | Description |
|------|-------------|
| **Real** | 64-bit double precision floating point number |
| **String** | Character strings for naming |
| **List** | Ordered, lazy, potentially infinite sequences |
| **Form** | Dictionary with inheritance (prototype chain) |
| **Function** | First-class functions with closures |
| **Ref** | Mutable container (the only mutable type) |

## Language Reference

### Basic Syntax

Expressions are sequences of words written in postfix form. All words are executed left to right. When a word is executed, it looks up the value bound to that word. If the value is a function, the function is applied with arguments taken from the stack. If the value is not a function, it is pushed onto the stack.

```
2 3 *  -->  6
```

Comments begin with a semicolon and continue to the end of the line:
```
; this is a comment
```

### Numbers

```
1  2.3  .5  7.              ;; standard notation
3.4e-3  1.7e4               ;; scientific notation
```

**Suffixes** scale the value:
```
pi  2pi  .5pi  .25pi        ;; pi (3.14159...)
1M  .5M                     ;; mega (x1,000,000)
4k  1.5k                    ;; kilo (x1,000)
8h                          ;; hecto (x100)
386c  702c                  ;; centi (x0.01)
53m  125m                   ;; milli (x0.001)
20u                         ;; micro (x0.000001)
```

**Inline fractions** (no spaces):
```
5/4  9/7  15/11  pi/4  7.5/4  1k/3
```

### Strings

Strings are enclosed in double quotes:
```
"This is a string"
"\tThis string begins with a tab and ends with a newline.\n"
```

### Words and Quotes

Words are sequences of characters delimited by spaces, brackets, braces, parentheses, or quote characters.

| Syntax | Behavior |
|--------|----------|
| `word` | Look up and apply the value bound to `word` |
| `` `word `` | Look up without applying (push function onto stack) |
| `'word` | Push the symbol itself onto the stack |
| `,name` | Pop object, look up `name` in it, push result |
| `.name` | Look up `name` in top object, apply it |
| `= x` | Bind top of stack to symbol `x` in current scope |

### Variable Binding

```
123 = x                     ;; bind 123 to x
```

**Destructuring from stack** (parentheses):
```
1 2 3 = (a b c)             ;; equivalent to: 1 2 3 = c = b = a
a b c  -->  1 2 3
```

**Destructuring from lists** (square brackets):
```
[1 2 3 4 5] = [a b c]
a b c  -->  1 2 3

#[1 2 3 4 5] = [a b c]      ;; also works for signals
a b c  -->  1 2 3
```

### Functions

Functions are a backslash followed by argument names, followed by a body in square brackets:

```
\a b [a b + a b *]          ;; function with args a, b

3 4 \a b [a b + a b *] !    -->  7 12   ;; apply with !

\a b [a b + a b *] = blub               ;; assign to word
3 4 blub  -->  7 12
```

Optional help string after arguments:
```
\a b
    "(a b --> sum product) returns the sum and product of a and b."
    [a b + a b *]
```

Unlike other concatenative languages, the body executes on an empty stack. Values from the calling stack are only accessible via named arguments.

### Lists

Lists are created by expressions within square brackets:
```
[1 2 3]
[1 2 + 3 4 *]  -->  [3 12]
[2 aa 3 ba]    -->  [2 3 2]
```

**Signals** (numeric lists) use `#`:
```
#[1 2 3]
#[1 2 + 3 4 *]  -->  #[3 12]
```

### Forms (Objects/Dictionaries)

Forms map keys to values with optional inheritance:
```
{ :a 1 :b 2 } = x           ;; bind 1 to key a, 2 to key b
{ x :c 3 } = y              ;; y inherits from x, adds key c
```

Key position is arbitrary within braces:
```
{1 2 :a :b} = x             ;; equivalent to { :a 1 :b 2 }
{:a :b 1 2} = x             ;; also equivalent
```

**Multiple inheritance**:
```
{:a 1} = a
{a :b 2} = b
{a :c 3} = c
{[b c] :d 4} = d            ;; inherit from b then c
```

### Auto-mapping

Many operators automatically map over lists:
```
0 4 to          -->  [0 1 2 3 4]
[0 2] 4 to      -->  [[0 1 2 3 4] [2 3 4]]
0 [2 3 4] to    -->  [[0 1 2] [0 1 2 3] [0 1 2 3 4]]
[0 7] [2 9] to  -->  [[0 1 2] [7 8 9]]
```

When multiple arguments are auto-mapped, the result is the length of the shortest list:
```
[0 1] [5 4 3] to  -->  [[0 1 2 3 4 5] [1 2 3 4]]
```

Works with infinite lists:
```
ord             -->  [1 2 3 4 5 ...]    ;; infinite integers
0 ord to        -->  [[0 1] [0 1 2] [0 1 2 3] ...]
```

### The "Each" Operator (@)

Apply operations at deeper levels:
```
[[1 2 3] [4 5 6]] reverse      -->  [[4 5 6] [1 2 3]]     ;; outer
[[1 2 3] [4 5 6]] @ reverse    -->  [[3 2 1] [6 5 4]]     ;; inner
```

**Outer products**:
```
[1 2] @ [10 20] +              -->  [[11 21] [12 22]]
[1 2] [10 20] @ +              -->  [[11 12] [21 22]]
```

**Ordered each** for nested loops:
```
[1 2] @1 [10 20] @2 2ple  -->  [[[1 10] [1 20]] [[2 10] [2 20]]]
```

**Deep mapping**:
```
[[[1 2 3] [4 5]] [[6 7] [8 9 10]]] @@ reverse
    -->  [[[3 2 1] [5 4]] [[7 6] [10 9 8]]]
```

### Multi-channel Expansion

Signal operators auto-map over streams but not signals:
```
;; Creates stereo with 1 Hz beating
[300 301] 0 saw .3 * play
```

### Reducing and Scanning

Add `/` for reduce, `\` for scan:
```
1 2 +           -->  3                  ;; normal addition
[1 2 3 4] +/    -->  10                 ;; sum (reduce)
[1 2 3 4] +\    -->  [1 3 6 10]         ;; accumulation (scan)
[1 2 3 4] */    -->  24                 ;; product
[1 2 3 4] *\    -->  [1 2 6 24]         ;; scan of multiplication
```

**Pairwise operator** with `^`:
```
[1 2 3 4 5 6] +^    -->  [1 3 5 7 9 11]     ;; pairwise sum
[7 9 16 20 1 5] -^  -->  [7 2 7 4 -19 4]    ;; pairwise difference
```

Note: `-^` and `+\` are inverses of each other.

## Getting Help

```
helpall              ;; list all functions
`someword help       ;; help for specific function (note backquote)
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

tests/               ;; Test suite (264 tests)
thirdparty/          ;; Dependencies (RtAudio, RtMidi, GoogleTest, etc.)
```

## Documentation

- **[ARCHITECTURE.md](ARCHITECTURE.md)** - VM execution model and internals
- **[CHANGELOG.md](CHANGELOG.md)** - Version history
- **[TODO.md](TODO.md)** - Planned features and improvements

## References

1. [Joy Programming Language](http://www.kevinalbrecht.com/code/joy-mirror/joy.html)
2. [Piccola](http://scg.unibe.ch/research/piccola)
3. [Nyquist](http://www.cs.cmu.edu/~music/nyquist/)
4. [SuperCollider](http://supercollider.sourceforge.net)
5. [Alan Perlis on APL](http://www.jsoftware.com/papers/perlis78.htm)
6. [Dylan Linearization](http://haahr.tempdomainname.com/dylan/linearization-oopsla96.html)

## License

GNU General Public License v3.0 - See [LICENSE](LICENSE) for details.

Copyright (C) 2019 James McCartney
