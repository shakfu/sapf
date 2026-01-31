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

#ifndef SAPF_OBJECTINLINES_HPP
#define SAPF_OBJECTINLINES_HPP

// This file contains inline implementations for V methods that require
// the complete Object class definition. It must be included AFTER all
// class definitions are complete.

//==============================================================================
// V inline implementations
//==============================================================================

inline O V::asObj() const
{
    if (!o) wrongType("asObj : v", "Object", *this);
    return o();
}

inline double V::asFloat() const { return o ? o->asFloat() : f; }
inline int64_t V::asInt() const { return o ? (int64_t)o->asFloat() : (int64_t)floor(f + .5); }

inline bool V::isFinite() const { return o ? o->isFinite() : false; }
inline bool V::done() const { return o ? o->done() : false; }
inline uint16_t V::takes() const { return o ? o->takes() : 0; }
inline uint16_t V::leaves() const { return o ? o->leaves() : 1; }

inline void V::SetNoEachOps() { if (o) o->SetNoEachOps(); }

inline int64_t V::length(Thread& th) { return !o ? 1 : o->length(th); }
inline Z V::atz(int64_t index) { return !o ? f : o->atz(index); }
inline Z V::wrapAtz(int64_t index) { return !o ? f : o->wrapAtz(index); }
inline Z V::foldAtz(int64_t index) { return !o ? f : o->foldAtz(index); }
inline Z V::clipAtz(int64_t index) { return !o ? f : o->clipAtz(index); }
inline V V::at(int64_t index) { return !o ? *this : o->at(index); }
inline V V::wrapAt(int64_t index) { return !o ? *this : o->wrapAt(index); }
inline V V::foldAt(int64_t index) { return !o ? *this : o->foldAt(index); }
inline V V::clipAt(int64_t index) { return !o ? *this : o->clipAt(index); }

inline V V::comma(Thread& th, Arg key)
{
    if (!o) wrongType("comma : v", "Object", *this);
    return o->comma(th, key);
}

inline bool V::dot(Thread& th, Arg key, V& ioValue)
{
    if (!o) return false;
    return o->dot(th, key, ioValue);
}

inline const char* V::TypeName() const { return !o ? "Real" : o->TypeName(); }
inline const char* V::OneLineHelp() const { return !o ? nullptr : o->OneLineHelp(); }
inline const char* V::GetAutoMapMask() const { return !o ? nullptr : o->GetAutoMapMask(); }

inline bool V::isTrue() const { return !o ? !(f == 0.) : o->isTrue(); }
inline bool V::isFalse() const { return !isTrue(); }

inline bool V::isRef() const { return o && o->isRef(); }
inline bool V::isZRef() const { return o && o->isZRef(); }
inline bool V::isPlug() const { return o && o->isPlug(); }
inline bool V::isZPlug() const { return o && o->isZPlug(); }
inline bool V::isString() const { return o && o->isString(); }
inline bool V::isArray() const { return o && o->isArray(); }

inline bool V::isFun() const { return o && o->isFun(); }
inline bool V::isPrim() const { return o && o->isPrim(); }
inline bool V::isFunOrPrim() const { return o && o->isFunOrPrim(); }

inline bool V::isSet() const { return o && o->isSet(); }
inline bool V::isTable() const { return o && o->isTable(); }
inline bool V::isGTable() const { return o && o->isGTable(); }
inline bool V::isForm() const { return o && o->isForm(); }
inline bool V::isGForm() const { return o && o->isGForm(); }
inline bool V::isList() const { return o && o->isList(); }
inline bool V::isVList() const { return o && o->isVList(); }
inline bool V::isZList() const { return o && o->isZList(); }
inline bool V::isEachOp() const { return o && o->isEachOp(); }

inline bool V::isZIn() const { return !o || o->isZIn(); }

inline V V::chase(Thread& th, int64_t n) { return !o ? f : o->chase(th, n); }

inline bool V::Identical(Arg v) const
{
    if (o) {
        if (!v.o) return false;
        return o->Identical(v.o());
    } else {
        if (v.o) return false;
        return f == v.f;
    }
}

inline bool V::Identical(const Object* _o) const
{
    if (!o) return false;
    return o->Identical(_o);
}

inline bool V::Equals(Thread& th, Arg v)
{
    if (!o && !v.o) return f == v.f;
    return !o ? v.o->Equals(th, *this) : o->Equals(th, v);
}

//==============================================================================
// Free function implementations
//==============================================================================

inline bool Equals(Thread& th, Arg a, Arg b)
{
    if (a.isReal()) {
        if (b.isReal()) return a.f == b.f;
        else return false;
    } else {
        return a.o->Equals(th, b);
    }
}

inline int Compare(Thread& th, Arg a, Arg b)
{
    if (a.isReal()) {
        if (b.isReal()) {
            if (a.f < b.f) return -1;
            if (a.f > b.f) return 1;
            if (a.f == b.f) return 0;
            // not a number
            return -2;
        } else return -1;
    } else {
        return a.o->Compare(th, b);
    }
}

//==============================================================================
// Object::dot implementation (requires V::msgSend)
//==============================================================================

inline bool Object::dot(Thread& th, Arg key, V& ioValue)
{
    V value;
    if (get(th, key, value)) {
        ioValue = value.msgSend(th, V(this));
        return true;
    } else {
        return false;
    }
}

inline V Object::comma(Thread& th, Arg key)
{
    return this->mustGet(th, key);
}

#endif // SAPF_OBJECTINLINES_HPP
