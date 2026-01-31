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

#ifndef SAPF_STRING_HPP
#define SAPF_STRING_HPP

#include "ObjectBase.hpp"
#include <cstring>
#include <cstdlib>

//==============================================================================
// String - Interned string/symbol class
//==============================================================================

class String : public Object
{
public:
    char* s;
    int32_t hash;
    String* nextSymbol;

    String(const char* str, int32_t inHash, String* nextSymbol = nullptr)
        : Object(), nextSymbol(nextSymbol)
    {
        s = strdup(str);
        hash = inHash;
    }

    String(const char* str)
        : Object(), nextSymbol(nullptr)
    {
        s = strdup(str);
        hash = ::Hash(s);
    }

    String(char* str, const char* dummy)
        : Object(), nextSymbol(nullptr)
    {
        s = str;
        hash = ::Hash(s);
    }

    virtual ~String() { free(s); }

    virtual const char* TypeName() const override { return "String"; }
    const char* cstr() const { return s; }

    virtual int64_t length(Thread& th) override { return strlen(s); }

    using Object::print;
    using Object::printDebug;
    virtual void print(Thread& th, std::string& out, int depth) override;
    virtual void printDebug(Thread& th, std::string& out, int depth) override;

    virtual bool isString() const override { return true; }

    virtual bool Equals(Thread& th, Arg v) override
    {
        if (v.Identical(this)) return true;
        return v.isString() && ((String*)v.o() == this ||
               (hash == ((String*)v.o())->hash && strcmp(s, ((String*)v.o())->s) == 0));
    }

    virtual int Compare(Thread& th, Arg b) override
    {
        if (b.isString()) { return strcmp(s, ((String*)b.o())->s); }
        return Object::Compare(th, b);
    }

    virtual int Hash() const override { return hash; }

    virtual V binaryOp(Thread& th, BinaryOp* op, Arg _b) override
    {
        if (_b.isString()) {
            return op->stringOp(this, (String*)_b.o());
        } else {
            wrongType("binaryOp with string.", "String", _b);
            return 0.; // never gets here
        }
    }
};

#endif // SAPF_STRING_HPP
