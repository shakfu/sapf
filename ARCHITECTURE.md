# SAPF Architecture

This document describes the internal architecture of SAPF (Sound As Pure Form), a stack-based audio programming language interpreter.

---

## Table of Contents

1. [Overview](#overview)
2. [VM Execution Model](#vm-execution-model)
3. [Type System and Values](#type-system-and-values)
4. [Reference Counting](#reference-counting)
5. [Threading Model](#threading-model)
6. [Audio Pipeline](#audio-pipeline)
7. [Cross-Platform Architecture](#cross-platform-architecture)
8. [Memory Layout](#memory-layout)

---

## Overview

SAPF is a postfix (Forth-like) language designed for audio synthesis and signal processing. Key characteristics:

- **Stack-based execution**: Operations consume and produce values on a stack
- **Lazy evaluation**: Audio signals are computed on-demand ("pull" model)
- **Immutable data**: Most values are immutable, enabling lock-free concurrency
- **Intrusive reference counting**: Memory managed via atomic reference counts

### Architecture Layers

```
+------------------------------------------+
|              CLI / REPL                  |
+------------------------------------------+
|              SapfEngine                  |
+------------------------------------------+
|    Parser    |    VM    |   Operations   |
+------------------------------------------+
|         Object System (V, P<T>)          |
+------------------------------------------+
|    Audio Backends   |   MIDI Backends    |
+------------------------------------------+
|         Platform Abstraction             |
+------------------------------------------+
```

---

## VM Execution Model

### Stack Machine

SAPF uses a stack-based virtual machine. Each `Thread` maintains:

```cpp
class Thread {
    std::vector<V> stack;    // Operand stack
    std::vector<V> local;    // Local variable storage
    size_t stackBase;        // Base for current function frame
    size_t localBase;        // Base for current locals frame
    Rate rate;               // Audio rate configuration
    // ...
};
```

### Execution Flow

1. **Parsing**: Source code is tokenized and compiled to opcodes
2. **Compilation**: AST is transformed into a `Code` object containing `Opcode` instructions
3. **Execution**: The VM executes opcodes sequentially

```
Source Code  -->  Parser  -->  Code (opcodes)  -->  VM.run()  -->  Result
```

### Opcodes

Operations are encoded as `Opcode` structures:

```cpp
struct Opcode {
    int op;      // Operation type
    V v;         // Associated value (if any)
};
```

Key opcode categories:
- **Push**: Push literal values onto stack
- **Apply**: Apply functions/primitives
- **Control**: Conditionals, loops
- **Stack manipulation**: dup, drop, swap, etc.

### Function Calls

When a function (`Fun`) is applied:

1. Arguments are popped from the stack
2. A new stack frame is established (`stackBase`, `localBase`)
3. Arguments become local variables
4. Function body executes
5. Frame is restored, results remain on stack

```cpp
void Fun::apply(Thread& th) {
    // Save current frame
    size_t saveStackBase = th.stackBase;
    size_t saveLocalBase = th.localBase;

    // Set up new frame
    th.setStackBase(NumArgs());
    th.setLocalBase();

    // Copy arguments to locals
    for (int i = 0; i < NumArgs(); ++i) {
        th.local.push_back(th.stack[th.stackBase + i]);
    }

    // Execute
    run(th);

    // Restore frame
    th.popLocals();
    th.setStackBaseTo(saveStackBase);
    th.setLocalBase(saveLocalBase);
}
```

---

## Type System and Values

### The V Type (Tagged Value)

`V` is a tagged union that can hold:
- A pointer to an `Object` (when `o` is non-null)
- A `double` value (when `o` is null)

```cpp
class V {
    P<Object> o;     // Object pointer (or null)
    union {
        double f;    // Float value
        int64_t i;   // Integer value
    };
};
```

Type checking:
```cpp
v.isReal()    // true if o is null (numeric value)
v.isObject()  // true if o is non-null
v.isList()    // true if object is a List
v.isForm()    // true if object is a Form
// etc.
```

### Object Hierarchy

```
Object (base class)
    |
    +-- String          // Interned strings/symbols
    +-- Ref             // Mutable reference to V
    +-- ZRef            // Mutable reference to Z (double)
    +-- Fun             // User-defined function
    +-- Prim            // Built-in primitive
    +-- List            // Lazy list (stream or signal)
    +-- Array           // Backing storage for List
    +-- Gen             // Generator (produces List elements)
    +-- Form            // Dictionary with inheritance
    +-- Table           // Key-value storage
    +-- GForm           // Global/mutable form
    +-- GTable          // Global/mutable table
    +-- Code            // Compiled bytecode
    +-- EachOp          // Auto-mapped operation
    +-- TreeNode        // Binary tree node for GTable
```

### List Types

SAPF has two list types distinguished by element type:

| Type | Element | Use Case |
|------|---------|----------|
| VList | `V` (any value) | General streams, heterogeneous data |
| ZList | `Z` (double) | Audio signals, numeric arrays |

Lists support lazy evaluation via generators:

```cpp
class List {
    P<Gen> mGen;      // Generator (produces elements on demand)
    P<Array> mArray;  // Materialized elements
    P<List> mNext;    // Next chunk (linked list of arrays)
};
```

---

## Reference Counting

### Overview

SAPF uses intrusive reference counting for memory management. Each `Object` contains its own reference count:

```cpp
class RCObj {
    mutable std::atomic<int32_t> refcount;

    void retain() const { ++refcount; }
    void release();
};
```

### Smart Pointer: P<T>

The `P<T>` template provides RAII-style reference management:

```cpp
template <class T>
class P {
    T* p_;

    // Construction: retains
    P(T* p) : p_(p) { if (p_) p_->retain(); }

    // Copy: retains
    P(P const& r) : p_(r.p_) { if (p_) p_->retain(); }

    // Move: no retain (transfers ownership)
    P(P&& r) : p_(r.p_) { r.p_ = nullptr; }

    // Destruction: releases
    ~P() { if (p_) p_->release(); }
};
```

### Release Semantics

When reference count reaches zero, `norefs()` is called to break cycles, then the object is deleted:

```cpp
void RCObj::release() {
    int32_t newRefCount = --refcount;
    if (newRefCount == 0)
        norefs();    // Virtual: clean up internal references
    if (newRefCount < 0)
        negrefcount();  // Error: double release
}
```

### Cycle Prevention

SAPF primarily uses immutable data structures, which prevents reference cycles. The few mutable types (`Ref`, `GForm`, `GTable`) use careful ownership patterns:

1. **Parent-to-child references**: Parents hold strong references to children
2. **No child-to-parent references**: Children do not reference parents
3. **Weak references where needed**: Some internal structures use raw pointers

Example: `Gen` holds a raw pointer to its output `List`, not a `P<List>`:
```cpp
class Gen : public Object {
    List* mOut;  // Raw pointer, not P<List>, to avoid cycle
};
```

### Thread Safety of Reference Counting

Reference count operations use `std::atomic<int32_t>`:
- `retain()` and `release()` are thread-safe
- Object destruction happens on the thread that decrements to zero
- Default memory ordering (`memory_order_seq_cst`) ensures visibility

---

## Threading Model

### Design Philosophy

SAPF achieves concurrency through **immutable data** rather than fine-grained locking:

> "Immutable data prevents deadlocks" - from README

Most operations create new objects rather than modifying existing ones. This allows multiple threads to share data without synchronization.

### Thread Isolation

Each `Thread` object is independent:
- Own stack and locals
- Own parser state
- Own random number generator
- Shares global `vm` state (read-mostly)

### Mutable State and Synchronization

The few mutable types use explicit synchronization:

#### 1. Ref (Mutable Value Reference)

Uses spinlock for atomic get/set:

```cpp
class Ref : public Object {
    Z z;
    O o;
    mutable SpinLockType mSpinLock;

    V deref() const {
        SpinLocker lock(mSpinLock);
        return o ? V(o) : V(z);
    }

    void set(Arg inV) {
        SpinLocker lock(mSpinLock);
        // ... update z and o
    }
};
```

#### 2. GTable (Global Mutable Table)

Uses atomic operations for lock-free reads, mutex for writes:

```cpp
class GTable : public Object {
    volatile std::atomic<TreeNode*> mTree;

    bool getInner(Arg key, V& value) const {
        // Lock-free read via atomic load
        TreeNode* tree = mTree.load();
        // ... traverse tree
    }

    bool putImpure(Arg key, Arg value) {
        // Creates new tree node, atomically swaps
    }
};
```

#### 3. Symbol Table

Global symbol interning uses atomic pointers:

```cpp
volatile std::atomic<String*> sSymbolTable[kSymbolTableSize];

String* getSymbol(const char* s) {
    // Lock-free lookup via atomic load
    // New symbols added with compare-and-swap
}
```

### Acknowledged Race Conditions

Some operations have documented race conditions that are accepted as trade-offs:

```cpp
// In Ref::chase()
virtual V chase(Thread& th, int64_t n) override {
    V v = deref();
    // race condition window.
    // may overwrite an intervening set from another thread,
    // but better than holding a lock.
    set(v.chase(th, n));
    return this;
}
```

### Platform Locks

Platform-specific spinlock abstraction in `PlatformLock.hpp`:

```cpp
#if defined(__APPLE__)
    using SpinLockType = os_unfair_lock;
    #define SPINLOCK_INIT OS_UNFAIR_LOCK_INIT
#else
    struct SpinLockType {
        std::atomic_flag flag = ATOMIC_FLAG_INIT;
    };
    #define SPINLOCK_INIT {}
#endif

class SpinLocker {
    SpinLockType& lock;
public:
    SpinLocker(SpinLockType& inLock) : lock(inLock) {
        spinlock_lock(&lock);
    }
    ~SpinLocker() {
        spinlock_unlock(&lock);
    }
};
```

### Thread Safety Guarantees

| Component | Thread Safety |
|-----------|---------------|
| `V` (value) | Not thread-safe (stack-local) |
| `P<T>` (smart pointer) | Reference counting is atomic |
| `Object` subclasses | Immutable after construction |
| `Ref`, `ZRef` | Thread-safe via spinlock |
| `GTable`, `GForm` | Thread-safe via atomics |
| `List` (lazy) | Thread-safe forcing via spinlock |
| `Gen` (generator) | Not thread-safe (single consumer) |

---

## Audio Pipeline

### Pull-Based Evaluation

Audio is computed lazily using a "pull" model:

1. Audio backend requests samples
2. Output `List` is forced
3. Generator's `pull()` method is called
4. Generator pulls from input generators
5. Samples propagate up the chain

```
Backend.play() --> List.force() --> Gen.pull() --> Input.pull() --> ...
```

### Block Processing

Audio is processed in blocks for efficiency:

```cpp
struct Rate {
    int blockSize;           // Samples per block (default: 512)
    double sampleRate;       // Samples per second (default: 96000)
    double nyquistRate;      // sampleRate / 2
    double invSampleRate;    // 1 / sampleRate
    // ...
};
```

### Generator Base Class

All signal generators inherit from `Gen`:

```cpp
class Gen : public Object {
    bool mDone;           // True when generator is exhausted
    List* mOut;           // Output list (raw pointer)
    int mBlockSize;       // Block size for this generator

    virtual void pull(Thread& th) = 0;  // Generate samples

    void produce(int shrinkBy);  // Mark output as ready
    void setDone();              // Mark as exhausted
};
```

### ZIn (Signal Input)

`ZIn` handles reading from signal sources:

```cpp
struct ZIn {
    P<List> mList;
    int mOffset;
    V mConstant;
    bool mIsConstant;

    // Get samples: returns true if done
    bool operator()(Thread& th, int& ioNum, int& outStride, Z*& outBuffer);
};
```

---

## Cross-Platform Architecture

SAPF supports macOS, Linux, and Windows through a layered abstraction strategy.

### Platform Abstraction Layer

**Location:** `include/sapf/platform/Platform.hpp`

```cpp
namespace Platform {
    void runAsync(std::function<void()> task);  // Background execution
    void runReplLoop(std::function<bool()> iteration);  // Event loop
    const char* getPlatformName();
    PlatformType getCurrentPlatform();  // MacOS, Linux, Windows, Unknown
}
```

| Platform | Async Strategy | Event Loop |
|----------|---------------|------------|
| macOS | GCD (dispatch_async) | CFRunLoop |
| Linux | std::thread | condition_variable |
| Windows | std::thread | condition_variable |

### Audio Backend Architecture

Multi-tier fallback strategy ensures audio works on all platforms:

```
+------------------+     +------------------+     +------------------+
|  CoreAudioBackend|     |  RtAudioBackend  |     | NullAudioBackend |
|    (macOS)       |     | (cross-platform) |     |   (fallback)     |
+------------------+     +------------------+     +------------------+
         |                       |                        |
         +-----------------------+------------------------+
                                 |
                         AudioBackend (interface)
                                 |
                    +------------+------------+
                    |                         |
               play(th, v)              record(th, v, filename)
```

**Backend Selection by Platform:**

| Platform | Primary | Secondary | Fallback |
|----------|---------|-----------|----------|
| macOS | CoreAudioBackend | RtAudioBackend | NullAudioBackend |
| Linux | RtAudioBackend | AlsaAudioBackend | NullAudioBackend |
| Windows | RtAudioBackend | - | NullAudioBackend |

**Recording Support:**

| Platform | Recording API | File Format |
|----------|--------------|-------------|
| macOS | ExtAudioFile (AudioToolbox) | WAV, AIFF, CAF |
| Linux | libsndfile | WAV |
| Windows | libsndfile | WAV |

### DSP Acceleration

**Location:** `include/sapf/AccelerateCompat.hpp`

```cpp
#if SAPF_ACCELERATE
    // macOS: Use Apple Accelerate.framework
    #include <Accelerate/Accelerate.h>
#else
    // Linux/Windows: FFTW3 with vDSP-compatible wrapper
    void vDSP_create_fftsetupD(...);
    void vDSP_fft_zopD(...);
    // ~400 lines of compatibility code
#endif
```

### MIDI Backend Architecture

| Platform | Primary | Fallback |
|----------|---------|----------|
| macOS | CoreMidiBackend | RtMidiBackend |
| Linux | RtMidiBackend | NullMidiBackend |
| Windows | RtMidiBackend | NullMidiBackend |

### Compiler Compatibility

MSVC-specific handling for GCC intrinsics:

```cpp
#if defined(_MSC_VER)
#include <intrin.h>
inline int sapf_clzll(unsigned long long x) {
    unsigned long index;
    _BitScanReverse64(&index, x);
    return 63 - (int)index;
}
#define __builtin_clzll(x) sapf_clzll(x)
#endif
```

---

## Memory Layout

### Object Memory

All objects inherit from `RCObj`:

```
+----------------+
| vtable ptr     |  8 bytes (virtual functions)
+----------------+
| refcount       |  4 bytes (atomic int32)
+----------------+
| scratch        |  1 byte
| elemType       |  1 byte
| finite         |  1 byte
| flags          |  1 byte
+----------------+
| subclass data  |  varies
+----------------+
```

### List Memory

Lists use a linked structure of arrays:

```
List                    Array
+-------------+        +-------------+
| mGen        |------->| Gen*        |
+-------------+        +-------------+
| mArray      |------->| mSize       |
+-------------+        | mCap        |
| mNext       |---+    | data[]      |
+-------------+   |    +-------------+
                  |
                  v
              List (next chunk)
              +-------------+
              | ...         |
```

### Stack Frame

During function execution:

```
stack:
+------------------+ <- stack.size()
| return values    |
+------------------+
| local temps      |
+------------------+ <- stackBase + numArgs
| arguments        |
+------------------+ <- stackBase
| caller's data    |
+------------------+

local:
+------------------+ <- local.size()
| local variables  |
+------------------+ <- localBase
| caller's locals  |
+------------------+
```

---

## Appendix: Key Files

### Core Engine

| File | Purpose |
|------|---------|
| `include/Object.hpp` | Core type system (V, Object, List, Form, etc.) |
| `include/VM.hpp` | Thread, VM, Rate, compilation |
| `include/UGen.hpp` | Audio generator base classes |
| `include/RCObj.hpp` | Reference counting base class |
| `include/PlatformLock.hpp` | Platform-specific synchronization |
| `src/engine/VM.cpp` | VM implementation |
| `src/engine/Parser.cpp` | Tokenizer and compiler |
| `src/engine/CoreOps.cpp` | Core stack/control operations |
| `src/engine/StreamOps.cpp` | List/stream operations |
| `src/engine/*UGens.cpp` | Audio unit generators |

### Platform Abstraction

| File | Purpose |
|------|---------|
| `include/sapf/platform/Platform.hpp` | Platform abstraction interface |
| `src/engine/platform/PlatformMac.cpp` | macOS implementation (GCD, CFRunLoop) |
| `src/engine/platform/PlatformUnix.cpp` | Linux implementation (std::thread) |
| `src/engine/platform/PlatformWindows.cpp` | Windows implementation (std::thread) |

### Audio Backends

| File | Purpose |
|------|---------|
| `include/sapf/AudioBackend.hpp` | Audio backend interface |
| `src/engine/backends/CoreAudioBackend.cpp` | macOS native audio (AudioToolbox) |
| `src/engine/backends/AlsaAudioBackend.cpp` | Linux native audio (ALSA + libsndfile) |
| `src/engine/backends/RtAudioBackend.cpp` | Cross-platform audio (RtAudio + libsndfile) |
| `src/engine/backends/NullAudioBackend.cpp` | Silent fallback |

### MIDI Backends

| File | Purpose |
|------|---------|
| `src/engine/backends/CoreMidiBackend.cpp` | macOS native MIDI |
| `src/engine/backends/RtMidiBackend.cpp` | Cross-platform MIDI |
| `src/engine/backends/NullMidiBackend.cpp` | Silent fallback |

### DSP and File I/O

| File | Purpose |
|------|---------|
| `include/sapf/AccelerateCompat.hpp` | FFT abstraction (Accelerate/FFTW3) |
| `src/engine/SoundFiles.cpp` | Audio file I/O (AudioToolbox/libsndfile) |
| `src/engine/dsp.cpp` | DSP utilities with MSVC compatibility |
