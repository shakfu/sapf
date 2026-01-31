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

#ifndef __Forward_h__
#define __Forward_h__

#include <stdint.h>

// Forward declarations for all major SAPF types
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

// Common type aliases
typedef Object* O;
typedef V const& Arg;

// Sample type - double precision for audio
#define SAMPLE_IS_DOUBLE 1
#if SAMPLE_IS_DOUBLE
typedef double Z;
#else
typedef float Z;
#endif

// Loop macros
#define LOOP(I,N) for (int I = 0;  i < (N); ++I)
#define LOOP2(I,S,N) for (int I = S;  i < (N); ++I)

// List item types
enum {
    itemTypeV,
    itemTypeZ
};

// Object flags
enum {
    flag_NoEachOps = 1
};

// Maximum arguments for operations
const int kMaxArgs = 16;

#endif // __Forward_h__
