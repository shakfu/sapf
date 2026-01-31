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

#ifndef SAPF_VALUE_HPP
#define SAPF_VALUE_HPP

#include "Forward.hpp"
#include <string>

//==============================================================================
// UnaryOp - Base class for unary operations
//==============================================================================

struct UnaryOp
{
    virtual ~UnaryOp() {}
    virtual const char *Name() = 0;
    virtual double op(double a) = 0;
    virtual void loop(Thread& th, int n, V *a, int astride, V *out);
    virtual void loopz(int n, const Z *a, int astride, Z *out) = 0;
};

//==============================================================================
// BinaryOp - Base class for binary operations
//==============================================================================

struct BinaryOp
{
    virtual ~BinaryOp() {}
    virtual const char *Name() = 0;
    virtual double op(double a, double b) = 0;

    virtual void loop(Thread& th, int n, V *a, int astride, V *b, int bstride, V *out);
    virtual void scan(Thread& th, int n, V& z, V *a, int astride, V *out);
    virtual void pairs(Thread& th, int n, V& z, V *a, int astride, V *out);
    virtual void reduce(Thread& th, int n, V& z, V *a, int astride);

    virtual void loopz(int n, const Z *a, int astride, const Z *b, int bstride, Z *out) = 0;
    virtual void scanz(int n, Z& z, Z *a, int astride, Z *out) { throw errUndefinedOperation; }
    virtual void pairsz(int n, Z& z, Z *a, int astride, Z *out) { throw errUndefinedOperation; }
    virtual void reducez(int n, Z& z, Z *a, int astride) { throw errUndefinedOperation; }

    virtual void loopzv(Thread& th, int n, Z *aa, int astride, V *bb, int bstride, V *out);
    virtual void loopvz(Thread& th, int n, V *aa, int astride, Z *bb, int bstride, V *out);

    virtual V makeVList(Thread& th, Arg a, Arg b);
    virtual V makeZList(Thread& th, Arg a, Arg b);

    virtual V stringOp(P<String> const& a, P<String> const& b);
};

struct BinaryOpLink : public BinaryOp
{
    virtual ~BinaryOpLink() {}
    virtual V makeVList(Thread& th, Arg a, Arg b);
    virtual V makeZList(Thread& th, Arg a, Arg b);
};

//==============================================================================
// V - Tagged Value
//
// A V can hold either:
// - A pointer to an Object (when o is non-null)
// - A double value (when o is null, value in f)
//
// This is the fundamental value type in SAPF.
//==============================================================================

class V
{
public:
    P<Object> o;
    union {
        double f;
        int64_t i;
    };

    // Constructors
    V() : o(nullptr), f(0.) {}
    V(O _o)  : o(_o), f(0.) {}
    V(double _f) : o(nullptr), f(_f) {}
    template <typename U> V(P<U> const& p) : o(p()), f(0.) {}

    // Setters
    template <typename T>
    void set(P<T> const& p) { o = p(); }
    void set(O _o) { o = _o; }
    void set(double _f) { o = nullptr; f = _f; }
    void set(Arg v) { o = v.o; f = v.f; }

    // Basic type checks (no Object dependency)
    bool isObject() const { return o; }
    bool isReal() const { return !o; }
    bool isZero() const { return !o && f == 0.; }

    // Methods requiring Object - declared here, defined in ObjectInlines.hpp
    O asObj() const;
    double asFloat() const;
    int64_t asInt() const;

    bool isFinite() const;
    bool done() const;

    void SetNoEachOps();

    const char* TypeName() const;
    const char* OneLineHelp() const;
    const char* GetAutoMapMask() const;

    void apply(Thread& th);
    V deref();
    Z derefz();

    int64_t length(Thread& th);
    Z atz(int64_t index);
    Z wrapAtz(int64_t index);
    Z foldAtz(int64_t index);
    Z clipAtz(int64_t index);
    V at(int64_t index);
    V wrapAt(int64_t index);
    V foldAt(int64_t index);
    V clipAt(int64_t index);
    V comma(Thread& th, Arg key);
    bool dot(Thread& th, Arg key, V& ioValue);
    V msgSend(Thread& th, Arg receiver);

    V mustGet(Thread& th, Arg key) const;
    bool get(Thread& th, Arg key, V& value) const;

    V chase(Thread& th, int64_t n);

    int Hash() const;

    uint16_t takes() const;
    uint16_t leaves() const;

    void print(Thread& th, std::string& out, int depth = 0) const;
    void printShort(Thread& th, std::string& out, int depth = 0) const;
    void printDebug(Thread& th, std::string& out, int depth = 0) const;

    void print(Thread& th, int depth = 0) const;
    void printShort(Thread& th, int depth = 0) const;
    void printDebug(Thread& th, int depth = 0) const;

    bool isTrue() const;
    bool isFalse() const;

    bool isRef() const;
    bool isZRef() const;
    bool isPlug() const;
    bool isZPlug() const;
    bool isString() const;
    bool isArray() const;

    bool isFun() const;
    bool isPrim() const;
    bool isFunOrPrim() const;

    bool isSet() const;
    bool isTable() const;
    bool isGTable() const;
    bool isForm() const;
    bool isGForm() const;
    bool isList() const;
    bool isVList() const;
    bool isZList() const;
    bool isEachOp() const;

    bool isZIn() const;

    bool Identical(Arg v) const;
    bool Identical(const Object* o) const;
    bool Equals(Thread& th, Arg v);

    // Math operations
    V unaryOp(Thread& th, UnaryOp* op) const;
    V binaryOp(Thread& th, BinaryOp* op, Arg _b) const;

    V binaryOpWithReal(Thread& th, BinaryOp* op, Z _a) const;
    V binaryOpWithVList(Thread& th, BinaryOp* op, List* _a) const;
    V binaryOpWithZList(Thread& th, BinaryOp* op, List* _a) const;
};

//==============================================================================
// Inline implementations that need complete V type
//==============================================================================

// Default implementation - V is now complete
inline V BinaryOp::stringOp(P<String> const& a, P<String> const& b) { throw errUndefinedOperation; }

#endif // SAPF_VALUE_HPP
