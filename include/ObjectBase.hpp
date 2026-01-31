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

#ifndef SAPF_OBJECTBASE_HPP
#define SAPF_OBJECTBASE_HPP

#include "Value.hpp"
#include "Hash.hpp"
#include <string>
#include <cstring>

//==============================================================================
// Object - Base class for all heap-allocated SAPF objects
//==============================================================================

class Object : public RCObj
{
public:
    uint8_t scratch;
    uint8_t elemType;
    uint8_t finite;
    uint8_t flags;

public:
    Object();
    virtual ~Object();

    // Comparison
    virtual int Compare(Thread& th, Arg b)
    {
        if (!b.o) return 1;

        Object* bb = b.o();
        int result = strcmp(TypeName(), bb->TypeName());
        if (result) return result;
        else if ((uintptr_t)this < (uintptr_t)bb) return -1;
        else if ((uintptr_t)this > (uintptr_t)bb) return 1;
        else return 0;
    }

    // Flags
    bool NoEachOps() const { return flags & flag_NoEachOps; }
    void SetNoEachOps() { flags |= flag_NoEachOps; }

    // Finiteness
    virtual bool isFinite() const { return finite; }
    void setFinite(bool b) { finite = b; }

    // Length and indexing
    virtual int64_t length(Thread& th) { return 1; }
    virtual Z atz(int64_t index) { return 0.; }
    virtual Z wrapAtz(int64_t index) { return 0.; }
    virtual Z foldAtz(int64_t index) { return 0.; }
    virtual Z clipAtz(int64_t index) { return 0.; }
    virtual V at(int64_t index) { return V(this); }
    virtual V at(Arg index) { return V(this); }
    virtual V wrapAt(int64_t index) { return V(this); }
    virtual V foldAt(int64_t index) { return V(this); }
    virtual V clipAt(int64_t index) { return V(this); }

    // Execution state
    virtual bool done() const { return false; }
    virtual uint16_t takes() const { return 0; }
    virtual uint16_t leaves() const { return 1; }

    // Help and documentation
    virtual const char* OneLineHelp() const { return nullptr; }
    virtual const char* GetAutoMapMask() const { return nullptr; }

    // Application and message passing
    virtual void apply(Thread& th);
    virtual bool dot(Thread& th, Arg key, V& ioValue);  // Defined in ObjectInlines.hpp
    virtual V comma(Thread& th, Arg key);               // Defined in ObjectInlines.hpp
    virtual V msgSend(Thread& th, Arg receiver) { return V(this); }

    // Dereferencing
    virtual V deref() const { return V(const_cast<O>(this)); }
    virtual Z derefz() const;
    virtual Z asFloat() const { return 0.; }

    // Dictionary access
    virtual V mustGet(Thread& th, Arg key) const { throw errNotFound; }
    virtual bool get(Thread& th, Arg key, V& value) const { return false; }

    // Chase (follow references)
    virtual V chase(Thread& th, int64_t n) { return this; }

    // Printing
    virtual void print(Thread& th, std::string& out, int depth = 0);
    virtual void printDebug(Thread& th, std::string& out, int depth = 0);
    virtual void printShort(Thread& th, std::string& out, int depth = 0) { print(th, out, depth); }

    void print(Thread& th, int depth = 0);
    void printDebug(Thread& th, int depth = 0);
    void printShort(Thread& th, int depth = 0);

    // Boolean conversion
    virtual bool isTrue() { return true; }
    bool isFalse() { return !isTrue(); }

    // Type predicates
    virtual bool isRef() const { return false; }
    virtual bool isZRef() const { return false; }
    virtual bool isPlug() const { return false; }
    virtual bool isZPlug() const { return false; }
    virtual bool isString() const { return false; }
    virtual bool isArray() const { return false; }
    virtual bool isZIn() const { return false; }
    virtual bool isFun() const { return false; }
    virtual bool isPrim() const { return false; }
    virtual bool isFunOrPrim() const { return false; }
    virtual bool isSet() const { return false; }
    virtual bool isTableMap() const { return false; }
    virtual bool isTable() const { return false; }
    virtual bool isGTable() const { return false; }
    virtual bool isForm() const { return false; }
    virtual bool isGForm() const { return false; }
    virtual bool isList() const { return false; }
    virtual bool isVList() const { return false; }
    virtual bool isZList() const { return false; }
    virtual bool isEachOp() const { return false; }

    // Hashing and equality
    virtual int Hash() const { return (int)::Hash64((uintptr_t)this); }
    virtual bool Identical(const Object* that) const { return this == that; }
    virtual bool Equals(Thread& th, Arg v)
    {
        return v.Identical(this);
    }

    // Math operations
    virtual V unaryOp(Thread& th, UnaryOp* op) { wrongType("unaryOp", "Real, or List", this); return V(); }
    virtual V binaryOp(Thread& th, BinaryOp* op, Arg _b) { wrongType("binaryOp", "Real, or List", this); return V(); }

    virtual V binaryOpWithReal(Thread& th, BinaryOp* op, Z _a) { wrongType("binaryOpWithReal", "Real, or List", this); return V(); }
    virtual V binaryOpWithVList(Thread& th, BinaryOp* op, List* _a) { wrongType("binaryOpWithVList", "Real, or List", this); return V(); }
    virtual V binaryOpWithZList(Thread& th, BinaryOp* op, List* _a) { wrongType("binaryOpWithZList", "Real, or List", this); return V(); }
};

#endif // SAPF_OBJECTBASE_HPP
