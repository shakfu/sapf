//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef SAPF_FORWARD_HPP
#define SAPF_FORWARD_HPP

#include <stdint.h>
#include <cmath>
#include "RCObj.hpp"
#include "rc_ptr.hpp"
#include "ErrorCodes.hpp"

//==============================================================================
// Forward Declarations
//==============================================================================

class VM;
class Thread;
class Object;
class String;
class Code;
class GForm;
class GTable;
class Form;
class Table;
class Fun;
class Prim;
class List;
class Gen;
class Array;
class V;
class TableMap;
class TreeNode;
class Ref;
class ZRef;
class FunDef;
class EachOp;
class Plug;
class ZPlug;

//==============================================================================
// Basic Type Definitions
//==============================================================================

// Object pointer alias
typedef Object* O;

// Argument type (const reference to V)
typedef V const& Arg;

// Sample type - double precision for audio
#define SAMPLE_IS_DOUBLE 1
#if SAMPLE_IS_DOUBLE
typedef double Z;
#else
typedef float Z;
#endif

// NaN constant
const double NaN = NAN;

//==============================================================================
// Flags and Constants
//==============================================================================

// Object flags
enum ObjectFlags {
    flag_NoEachOps = 1
};

// List item types
enum ItemType {
    itemTypeV = 0,
    itemTypeZ = 1
};

// Maximum arguments for operations
const int kMaxArgs = 16;

//==============================================================================
// Loop Macros
//==============================================================================

#define LOOP(I,N) for (int I = 0; I < (N); ++I)
#define LOOP2(I,S,N) for (int I = S; I < (N); ++I)

//==============================================================================
// Primitive Function Type
//==============================================================================

typedef void (*PrimFun)(Thread& th, Prim*);

//==============================================================================
// Error Functions
//==============================================================================

[[noreturn]] void wrongType(const char* msg, const char* expected, Arg got);
[[noreturn]] void syntaxError(const char* msg);
[[noreturn]] void indefiniteOp(const char* msg1, const char* msg2);
[[noreturn]] void notFound(Arg key);

//==============================================================================
// Post function (Max SDK compatible)
//==============================================================================

extern "C" void post(const char* fmt, ...);

#endif // SAPF_FORWARD_HPP
