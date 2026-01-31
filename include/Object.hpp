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

#ifndef __Object_h__
#define __Object_h__

//==============================================================================
// Include modular headers in dependency order
//==============================================================================

// Forward declarations and basic types
#include "Forward.hpp"

// Value type (V) - declaration only, no inline implementations
#include "Value.hpp"

// Object base class
#include "ObjectBase.hpp"

// String class
#include "String.hpp"

//==============================================================================
// Additional includes needed by remaining classes
//==============================================================================

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include "MathFuns.hpp"
#include "PlatformLock.hpp"

#define COLLECT_MINFO 1

//==============================================================================
// Ref - Mutable reference to a value
//==============================================================================

class Ref : public Object
{
	Z z;
	O o;
    mutable SpinLockType mSpinLock = SPINLOCK_INIT;
public:

	Ref(V inV) : z(inV.f), o(inV.o()) { if (o) o->retain(); }

	virtual const char* TypeName() const override { return "Ref"; }

	virtual void print(Thread& th, std::string& out, int depth) override;

	virtual bool isRef() const override { return true; }

	virtual bool Equals(Thread& th, Arg that) override
	{
		if (that.Identical(this)) return true;
		if (!that.isRef()) return false;
		Ref* r = (Ref*)that.o();
		V a = deref();
		V b = r->deref();
		return a.Equals(th, b);
	}

	virtual void set(Arg inV);
	virtual V deref() const override;
	virtual Z derefz() const override { return deref().asFloat(); }
	virtual Z asFloat() const override
	{
		V v = deref();
		return v.asFloat();
	}

	virtual V chase(Thread& th, int64_t n) override {
		V v = deref();
		// race condition window.
		// may overwrite an intervening set from another thread, but better than holding a lock.
		set(v.chase(th, n));
		return this;
	}

	virtual V unaryOp(Thread& th, UnaryOp* op) override
	{
		return deref().unaryOp(th, op);
	}
	virtual V binaryOp(Thread& th, BinaryOp* op, Arg _b) override
	{
		return deref().binaryOp(th, op, _b);
	}

	virtual V binaryOpWithReal(Thread& th, BinaryOp* op, Z _a) override
	{
		return deref().binaryOpWithReal(th, op, _a);
	}
	virtual V binaryOpWithVList(Thread& th, BinaryOp* op, List* _a) override
	{
		return deref().binaryOpWithVList(th, op, _a);
	}
	virtual V binaryOpWithZList(Thread& th, BinaryOp* op, List* _a) override
	{
		return deref().binaryOpWithZList(th, op, _a);
	}
};

//==============================================================================
// ZRef - Mutable reference to a Z (sample) value
//==============================================================================

class ZRef : public Object
{
public:
	Z z;

	ZRef(Z inZ) : z(inZ) {}

	virtual const char* TypeName() const override { return "ZRef"; }

	virtual void print(Thread& th, std::string& out, int depth) override;

	virtual bool isZRef() const override { return true; }

	virtual bool Equals(Thread& th, Arg that) override
	{
		if (!that.isZRef()) return false;
		return z == ((ZRef*)that.o())->z;
	}

	virtual void set(Z inZ);
	virtual V deref() const override;
	virtual Z derefz() const override;

	virtual V chase(Thread& th, int64_t n) override {
		return V(z);
	}


	virtual V unaryOp(Thread& th, UnaryOp* op) override
	{
		return deref().unaryOp(th, op);
	}
	virtual V binaryOp(Thread& th, BinaryOp* op, Arg _b) override
	{
		return deref().binaryOp(th, op, _b);
	}

	virtual V binaryOpWithReal(Thread& th, BinaryOp* op, Z _a) override
	{
		return deref().binaryOpWithReal(th, op, _a);
	}
	virtual V binaryOpWithVList(Thread& th, BinaryOp* op, List* _a) override
	{
		return deref().binaryOpWithVList(th, op, _a);
	}
	virtual V binaryOpWithZList(Thread& th, BinaryOp* op, List* _a) override
	{
		return deref().binaryOpWithZList(th, op, _a);
	}
};

//==============================================================================
// FunDef - Function definition (compiled code + metadata)
//==============================================================================

class FunDef : public Object
{
public:
	P<Code> mCode;
	std::vector<P<String> > mArgNames;
	uint16_t mNumArgs;
	uint16_t mNumLocals;
	uint16_t mNumVars;
 	uint16_t mLeaves;
	P<GForm> mWorkspace;
    P<String> mHelp;

	FunDef(Thread& th, P<Code> const& inCode, uint16_t inNumArgs, uint16_t inNumLocals, uint16_t inNumVars, P<String> const& inHelp);

	virtual const char* TypeName() const override { return "FunDef"; }
	virtual const char* OneLineHelp() const override { return mHelp() ? mHelp->cstr() : nullptr; }
	P<GForm> Workspace() const { return mWorkspace; }

};

//==============================================================================
// Fun - Function instance (closure)
//==============================================================================

class Fun : public Object
{
public:
	P<FunDef> mDef;
	std::vector<V> mVars;
	P<GForm> mWorkspace;

	Fun(Thread& th, FunDef* def);
	virtual ~Fun();

	virtual const char* TypeName() const override { return "Fun"; }
	virtual const char* OneLineHelp() const override { return mDef->OneLineHelp(); }
	P<GForm>& Workspace() { return mWorkspace; }
	uint16_t NumArgs() const { return mDef->mNumArgs; }
	uint16_t NumLocals() const { return mDef->mNumLocals; }
	uint16_t NumVars() const { return mDef->mNumVars; }
	uint16_t Leaves() const { return mDef->mLeaves; }

	virtual uint16_t takes() const override { return NumArgs(); }
	virtual uint16_t leaves() const override { return Leaves(); }

	virtual bool isFun() const override { return true; }
	virtual bool isFunOrPrim() const override { return true; }
	virtual bool isFinite() const override { return false; }

	virtual V msgSend(Thread& th, Arg receiver) override;
	virtual void apply(Thread& th) override;
	void run(Thread& th);
	void runREPL(Thread& th);
};

//==============================================================================
// Prim - Primitive (built-in) function
//==============================================================================

class Prim : public Object
{
public:
	PrimFun prim;
	V v;
	const char* mName;
	const char* mHelp;
	uint16_t mTakes;
	uint16_t mLeaves;

	Prim(PrimFun _primFun, Arg _v, uint16_t takes, uint16_t leaves, const char* name, const char* help)
		: Object(), prim(_primFun), v(_v), mName(name), mHelp(help), mTakes(takes), mLeaves(leaves) {}

	virtual const char* TypeName() const override { return "Prim"; }
	virtual const char* OneLineHelp() const override { return mHelp; }
	virtual const char* GetAutoMapMask() const override;

	virtual bool isPrim() const override { return true; }
	virtual bool isFunOrPrim() const override { return true; }

	virtual V msgSend(Thread& th, Arg receiver) override;
	virtual void apply(Thread& th) override;
	virtual void apply_n(Thread& th, size_t n);

	uint16_t Takes() const { return mTakes; }
	uint16_t Leaves() const { return mLeaves; }

	virtual uint16_t takes() const override { return Takes(); }
	virtual uint16_t leaves() const override { return Leaves(); }

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override;
	using Object::printDebug;
	virtual void printDebug(Thread& th, std::string& out, int depth) override;
};

//==============================================================================
// EachOp - Each operation wrapper
//==============================================================================

class EachOp : public Object
{
public:
	V v;
	int32_t mask;

	EachOp(Arg inV, int inMask)
		: v(inV), mask(inMask) {}


	virtual const char* TypeName() const override { return "EachOp"; }

	virtual bool isEachOp() const override { return true; }

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override;
};

//==============================================================================
// TreeNode - Immutable tree node for persistent data structures
//==============================================================================

class TreeNode : public Object
{
public:
	V mKey;
	V mValue;
    int64_t mHash;
    int64_t mSerialNumber;
	volatile std::atomic<TreeNode*> mLeft;
	volatile std::atomic<TreeNode*> mRight;

	TreeNode(Arg inKey, int64_t inKeyHash, Arg inValue, int64_t inSerialNumber,
        TreeNode* inLeft, TreeNode* inRight)
        : mKey(inKey), mValue(inValue), mHash(inKeyHash), mSerialNumber(inSerialNumber)
    {
		if (inLeft) inLeft->retain();
		if (inRight) inRight->retain();
        mLeft.store(inLeft);
        mRight.store(inRight);
    }

    ~TreeNode()
    {
        auto left = mLeft.load();
        auto right = mRight.load();
        if (left) left->release();
        if (right) right->release();
    }

	virtual const char* TypeName() const override { return "TreeNode"; }

    TreeNode* putPure(Arg inKey, int64_t inKeyHash, Arg inValue);

	void getAll(std::vector<P<TreeNode> >& vec);
};

//==============================================================================
// GTable - Global (growable) table with atomic tree
//==============================================================================

class GTable : public Object
{
    volatile std::atomic<TreeNode*> mTree;
	GTable(const GTable& that) {}
public:

	GTable(TreeNode* inTree = nullptr) {
        if (inTree) inTree->retain();
        mTree.store(inTree);
    }

	virtual ~GTable() {
        auto tree = mTree.load();
        if (tree) tree->release();
    }

	virtual const char* TypeName() const override { return "GTable"; }

	virtual bool isGTable() const override { return true; }
	virtual bool Equals(Thread& th, Arg v) override;

	virtual bool get(Thread& th, Arg key, V& value) const override;
    bool getInner(Arg inKey, V& outValue) const;
	virtual V mustGet(Thread& th, Arg key) const override;

	bool putImpure(Arg key, Arg value);
	GTable* putPure(Arg key, int64_t keyHash, Arg value);

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override;
	virtual void printSomethingIWant(Thread& th, std::string& out, int depth);

    const TreeNode* tree() { return mTree.load(); }

	std::vector<P<TreeNode> > sorted() const;
};

//==============================================================================
// GForm - Global form (prototype chain with GTable)
//==============================================================================

class GForm : public Object
{
public:
	P<GTable> mTable;
	P<GForm> mNextForm;

	GForm(P<GTable> const& inTable, P<GForm> const& inNext = nullptr);
	GForm(P<GForm> const& inNext = nullptr);

	virtual const char* TypeName() const override { return "GForm"; }

	virtual bool isGForm() const override { return true; }
	virtual bool Equals(Thread& th, Arg v) override
	{
		if (!v.isGForm()) return false;
		GForm* that = (GForm*)v.o();

        // fail cheaply first
        if (mNextForm() == 0 && that->mNextForm() != 0) return false;
		if (mNextForm() != 0 && that->mNextForm() == 0) return false;

		if (!mTable->Equals(th, that->mTable())) return false;
		if (mNextForm() == 0 && that->mNextForm() == 0) return true;

		return mNextForm->Equals(th, that->mNextForm());
	}

	virtual bool get(Thread& th, Arg key, V& value) const override;

	GForm* putImpure(Arg inKey, Arg inValue);
    GForm* putPure(Arg inKey, Arg inValue);

	virtual V mustGet(Thread& th, Arg key) const override;

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override;
};

//==============================================================================
// TableMap - Hash map for table keys
//==============================================================================

class TableMap : public Object
{
public:
	size_t mSize;
	size_t mMask;
	size_t* mIndices;
	V* mKeys;

	TableMap(size_t inSize);
	TableMap(Arg inKey); // one item table map
	~TableMap();

	bool getIndex(Arg inKey, int64_t inKeyHash, size_t& outIndex);
    void put(size_t inIndex, Arg inKey, int64_t inKeyHash);

	virtual const char* TypeName() const override { return "TableMap"; }

	virtual bool isTableMap() const override { return true; }

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override;
};

//==============================================================================
// Table - Immutable table with hash map
//==============================================================================

class Table : public Object
{
public:
	P<TableMap> mMap;
	V* mValues;

	Table(P<TableMap> const& inMap);
	~Table();

	virtual bool Equals(Thread& th, Arg v) override;

	bool getWithHash(Thread& th, Arg key, int64_t hash, V& value) const;
    void put(size_t inIndex, Arg inValue);

	virtual const char* TypeName() const override { return "Table"; }
	virtual bool isTable() const override { return true; }

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override;

	P<Table> chaseTable(Thread& th, int64_t n);
};

//==============================================================================
// Form - Immutable form (prototype chain)
//==============================================================================

class Form : public Object
{
public:
	P<Table> mTable;
	P<Form> mNextForm;

	Form(P<Table> const& inTable, P<Form> const& inNext = nullptr);

	virtual const char* TypeName() const override { return "Form"; }

	virtual bool isForm() const override { return true; }
	virtual bool Equals(Thread& th, Arg v) override
	{
		if (v.Identical(this)) return true;
		if (!v.isForm()) return false;
		Form* that = (Form*)v.o();

        // fail cheaply first
        if (mNextForm() == 0 && that->mNextForm() != 0) return false;
		if (mNextForm() != 0 && that->mNextForm() == 0) return false;

		if (!mTable->Equals(th, that->mTable())) return false;
		if (mNextForm() == 0 && that->mNextForm() == 0) return true;

		return mNextForm->Equals(th, that->mNextForm());
	}

	virtual bool get(Thread& th, Arg key, V& value) const override;

    void put(int64_t inIndex, Arg inValue);

	virtual V mustGet(Thread& th, Arg key) const override;

	virtual V chase(Thread& th, int64_t n) override { return chaseForm(th, n); }
	P<Form> chaseForm(Thread& th, int64_t n);

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override;
};

//==============================================================================
// In, VIn, ZIn, BothIn - Input stream abstractions
//==============================================================================

struct In
{
	P<List> mList;
	int mOffset;
	V mConstant;
	bool mIsConstant;
	bool mDone = false;

	In();
	In(V inValue);

	bool isConstant() const { return mIsConstant; }
	bool isZero() const { return isConstant() && mConstant.isZero(); }

	void advance(int inNum);
	bool done() const { return mDone; }
};

struct VIn : In
{
	VIn();
	VIn(Arg inValue);

	void set(Arg v);
	void setConstant(Arg v);
	bool operator()(Thread& th, int& ioNum, int& outStride, V*& outBuffer);
    bool one(Thread& th, V& v);
	bool link(Thread& th, List* inList);
};

struct ZIn : In
{
	bool mOnce = true;

	ZIn();
	ZIn(Arg inValue);

	void set(Arg v);
	bool operator()(Thread& th, int& ioNum, int& outStride, Z*& outBuffer);
    bool onez(Thread& th, Z& z);
    bool peek(Thread& th, Z& z);
	bool fill(Thread& th, int& ioNum, Z* outBuffer, int outStride);
	bool fill(Thread& th, int& ioNum, float* outBuffer, int outStride);
	bool mix(Thread& th, int& ioNum, Z* outBuffer);
	bool bench(Thread& th, int& ioNum);
	bool link(Thread& th, List* inList);

	bool fillSegment(Thread& th, int inNum, Z* outBuffer);
	void hop(Thread& th, int framesToAdvance);
};


struct BothIn : In
{
	BothIn();
	BothIn(Arg inValue);

	void set(Arg v);
	void setv(Arg v);
	void setConstant(Arg v);
    bool one(Thread& th, V& v);
    bool onez(Thread& th, Z& z);
	bool onei(Thread& th, int64_t& i);
};

//==============================================================================
// Gen - Generator base class
//==============================================================================

class Gen : public Object
{
public:
	friend class VIn;
	friend class ZIn;
	bool mDone;
	List* mOut;
	int mBlockSize;

	Gen(Thread& th, int inItemType, bool finite = false);
	virtual ~Gen();

	void setOut(List* inOut) { if (!mOut) mOut = inOut; }

	virtual void pull(Thread& th) = 0;

	void setDone();
	void end();
	bool done() const { return mDone; }

	void produce(int shrinkBy);

	int blockSize() const { return mBlockSize; }
};

//==============================================================================
// Plug - Mutable input plug for VIn
//==============================================================================

class Plug : public Object
{
	VIn in;
    mutable SpinLockType mSpinLock = SPINLOCK_INIT;
	int mChangeCount;
public:

	Plug(Arg inV) : in(inV), mChangeCount(0) {}

	virtual const char* TypeName() const override { return "Plug"; }

	virtual bool isPlug() const override { return true; }

	virtual bool Equals(Thread& th, Arg that) override
	{
		if (that.Identical(this)) return true;
		return false;
	}

	void setPlug(Arg inV);
	void setPlug(const VIn& inVIn, int inChangeCount);
	void getPlug(VIn& outVIn, int& outChangeCount);
};

//==============================================================================
// ZPlug - Mutable input plug for ZIn
//==============================================================================

class ZPlug : public Object
{
	ZIn in;
    mutable SpinLockType mSpinLock = SPINLOCK_INIT;
	int mChangeCount;
public:

	ZPlug(Arg inV) : in(inV), mChangeCount(0) {}

	virtual const char* TypeName() const override { return "ZPlug"; }

	virtual bool isZPlug() const override { return true; }

	virtual bool Equals(Thread& th, Arg that) override
	{
		if (that.Identical(this)) return true;
		return false;
	}

	void setPlug(Arg inV);
	void setPlug(const ZIn& inZIn, int inChangeCount);
	void getPlug(ZIn& outZIn, int& outChangeCount);
};

//==============================================================================
// Array - Dynamic array of V or Z values
//==============================================================================

class Array : public Object
{
	int64_t mSize;
	int64_t mCap;
	union {
		void* p;
		V* vv;
		Z* zz;
	};

public:

	Array(int inItemType, int64_t inCap) : mSize(0), mCap(0), p(0)
	{
		elemType = inItemType;
		alloc(std::max(int64_t(1), inCap));
	}

	virtual ~Array();

	virtual const char* TypeName() const override { return "Array"; }
	virtual bool isArray() const override { return true; }

	bool isV() const { return elemType == itemTypeV; }
	bool isZ() const { return elemType == itemTypeZ; }

    V* v() { return vv; }
    Z* z() { return zz; }

	size_t elemSize() { return isV() ? sizeof(V) : sizeof(Z); }
	void alloc(int64_t inCap);

	int64_t size() const { return mSize; }
    void setSize(size_t inSize) { mSize = inSize; }
    void addSize(size_t inDelta) { mSize += inDelta; }

	void add(Arg value);
	void put(int64_t i, Arg inItem);
	void addAll(Array* a);

	void addz(Z value);
	void putz(int64_t i, Z inItem);

	using Object::at;
	V _at(int64_t i); // no bounds check
	Z _atz(int64_t i); // no bounds check
	V at(int64_t i) override;
	V wrapAt(int64_t i) override;
	V clipAt(int64_t i) override;
	V foldAt(int64_t i) override;

	Z atz(int64_t i) override;
	Z wrapAtz(int64_t i) override;
	Z clipAtz(int64_t i) override;
	Z foldAtz(int64_t i) override;

	int ItemType() const { return elemType; }

	virtual int Compare(Thread& th, Arg b) override
	{
		if (b.isArray() && ItemType() == ((Array*)b.o())->ItemType()) {
			P<Array> bb((Array*)b.o());
			if (isV()) {
				int64_t n = std::min(size(), bb->size());
				for (int64_t i = 0; i < n; ++i) {
					int result = ::Compare(th, at(i), bb->at(i));
					if (result) return result;
				}
				return size() < bb->size();
			} else {
				int64_t n = std::min(size(), bb->size());
				for (int64_t i = 0; i < n; ++i) {
					int result = ::Compare(th, atz(i), bb->atz(i));
					if (result) return result;
				}
				return size() < bb->size();
			}
		} else {
			return Object::Compare(th, b);
		}
	}

};

#define ASSERT_PACKED  assert(isPacked());

//==============================================================================
// List - Lazy list with generator support
//==============================================================================

class List : public Object
{
	P<List> mNext;
public:
    mutable SpinLockType mSpinLock = SPINLOCK_INIT;
	P<Gen> mGen;
	P<Array> mArray;

	List(int inItemType);
	List(int inItemType, int64_t inCap);

	List(P<Gen> const& inGen);

	List(P<Array> const& inArray);

	List(P<Array> const& inArray, P<List> const& inNext);

	virtual ~List();

	P<List>& next() { return mNext; }
	List* nextp() const { return mNext(); }

	virtual const char* TypeName() const override { return  isV() ? "VList" : "ZList"; }

	virtual bool isList() const override { return true; }
	virtual bool isVList() const override { return elemType == itemTypeV; }
	virtual bool isZList() const override { return elemType == itemTypeZ; }
	virtual bool isZIn() const override { return isZList(); }

	int ItemType() const { return elemType; }

	bool isThunk() const { return mGen; }
	bool isFilled() const { return mArray; }
	bool isEnd() const { return !mArray->size() && !mNext; }
	bool isV() const { return elemType == itemTypeV; }
	bool isZ() const { return elemType == itemTypeZ; }
	bool isPacked() const { return !mNext && !mGen; }

	virtual int64_t length(Thread& th) override;

	V* fulfill(int n);
	V* fulfill_link(int n, P<List> const& next);
	V* fulfill(P<Array> const& inArray);
	Z* fulfillz(int n);
	Z* fulfillz_link(int n, P<List> const& next);
	Z* fulfillz(P<Array> const& inArray);
	void link(Thread& th, List* inList);
	void end();

	List* pack(Thread& th);
	List* packz(Thread& th);
	List* pack(Thread& th, int limit);
	List* packSome(Thread& th, int64_t& limit);
	void forceAll(Thread& th);
	void force(Thread& th);

	int64_t fillz(Thread& th, int64_t n, Z* z);

	virtual V comma(Thread& th, Arg key) override;
	virtual bool dot(Thread& th, Arg key, V& ioValue) override;

	virtual bool Equals(Thread& th, Arg v) override;

	// these assume the list is packed
	void put(int64_t index, Arg value) { ASSERT_PACKED mArray->put(index, value); }
	void add(Arg value) { ASSERT_PACKED mArray->add(value); }

	void putz(int64_t index, Z value) { ASSERT_PACKED mArray->put(index, value); }
	void addz(Z value) { ASSERT_PACKED mArray->addz(value); }

	using Object::at;
	V at(int64_t i) override     { ASSERT_PACKED return mArray->at(i); }
	V wrapAt(int64_t i) override { ASSERT_PACKED return mArray->wrapAt(i); }
	V clipAt(int64_t i) override { ASSERT_PACKED return mArray->clipAt(i); }
	V foldAt(int64_t i) override { ASSERT_PACKED return mArray->foldAt(i); }

	Z atz(int64_t i) override     { ASSERT_PACKED return mArray->atz(i); }
	Z wrapAtz(int64_t i) override { ASSERT_PACKED return mArray->wrapAtz(i); }
	Z clipAtz(int64_t i) override { ASSERT_PACKED return mArray->clipAtz(i); }
	Z foldAtz(int64_t i) override { ASSERT_PACKED return mArray->foldAtz(i); }

	virtual V chase(Thread& th, int64_t n) override;

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override;

	virtual V unaryOp(Thread& th, UnaryOp* op) override;
	virtual V binaryOp(Thread& th, BinaryOp* op, Arg _b) override
	{
		if (isVList())
			return _b.binaryOpWithVList(th, op, this);
		else
			return _b.binaryOpWithZList(th, op, this);
	}

	virtual V binaryOpWithReal(Thread& th, BinaryOp* op, Z _a) override;
	virtual V binaryOpWithVList(Thread& th, BinaryOp* op, List* _a) override;
	virtual V binaryOpWithZList(Thread& th, BinaryOp* op, List* _a) override;

	virtual int Compare(Thread& th, Arg that) override
	{
		if (that.isList() && isFinite() && that.isFinite() && ItemType() == ((List*)that.o())->ItemType()) {

			if (isV()) {
				VIn aa(this);
				VIn bb(that);

				while (1) {
					V a, b;
					if (aa.one(th, a)) {
						if (bb.one(th, b)) return 0;
						return -1;
					}
					if (bb.one(th, b)) return 1;
					int result = ::Compare(th, a, b);
					if (result) return result;
				}
			} else {
				ZIn aa(this);
				ZIn bb(that);

				while (1) {
					Z a, b;
					if (aa.onez(th, a)) {
						if (bb.onez(th, b)) return 0;
						return -1;
					}
					if (bb.onez(th, b)) return 1;
					int result = ::Compare(th, a, b);
					if (result) return result;
				}
			}
		} else {
			return Object::Compare(th, that);
		}
	}

};

void dumpList(List const* list);

//==============================================================================
// Opcode - VM instruction
//==============================================================================

struct Opcode
{
	Opcode() : op(0) {}
	Opcode(int _op, Arg _v) : op(_op), v(_v) {}

	int op;
	V v;
};

//==============================================================================
// Code - Compiled bytecode
//==============================================================================

class Code : public Object
{
public:
	std::vector<Opcode> ops;
	std::vector<V> keys;

	Code(int64_t capacity) : Object() { ops.reserve(capacity); }
	virtual ~Code();

	virtual const char* TypeName() const override { return "Code"; }

	virtual bool isCode() const { return true; }

	void shrinkToFit();

	int64_t size() { return ops.size(); }

	Opcode* getOps() { return &ops[0]; }

	void addAll(P<Code> const& that);

	void add(int op, Arg v);
	void add(int op, double f);

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override;

	void decompile(Thread& th, std::string& out);
};

//==============================================================================
// Inline implementations that need complete Array type
//==============================================================================

inline V Array::_at(int64_t i)
{
	if (isV()) return vv[i];
	else return V(zz[i]);
}

inline Z Array::_atz(int64_t i)
{
	if (isZ()) return zz[i];
	else return vv[i].asFloat();
}

inline V Array::at(int64_t i)
{
	if (mSize == 0) return V(0.);
	if (i < 0 || i >= mSize) return V(0.);
	return _at(i);
}

inline Z Array::atz(int64_t i)
{
	if (mSize == 0) return 0.;
	if (i < 0 || i >= mSize) return 0.;
	return _atz(i);
}

inline V Array::wrapAt(int64_t i)
{
	if (mSize == 0) return V(0.);
	i = sc_imod(i, mSize);
	return _at(i);
}

inline Z Array::wrapAtz(int64_t i)
{
	if (mSize == 0) return 0.;
	i = sc_imod(i, mSize);
	return _atz(i);
}

inline V Array::clipAt(int64_t i)
{
	if (mSize == 0) return V(0.);
	if (i < 0) i = 0;
	else if (i >= mSize) i = mSize - 1;
	return _at(i);
}

inline Z Array::clipAtz(int64_t i)
{
	if (mSize == 0) return 0.;
	if (i < 0) i = 0;
	else if (i >= mSize) i = mSize - 1;
	return _atz(i);
}

inline V Array::foldAt(int64_t i)
{
	if (mSize == 0) return V(0.);
	i = sc_fold(i, 0, mSize-1);
	return _at(i);
}

inline Z Array::foldAtz(int64_t i)
{
	if (mSize == 0) return 0.;
	i = sc_fold(i, 0, mSize-1);
	return _atz(i);
}

//==============================================================================
// Utility functions
//==============================================================================

inline bool mostFinite(Arg a, Arg b)
{
	return a.isFinite() || b.isFinite();
}

inline bool mostFinite(Arg a, Arg b, Arg c)
{
	return a.isFinite() || b.isFinite() || c.isFinite();
}

inline bool mostFinite(Arg a, Arg b, Arg c, Arg d)
{
	return a.isFinite() || b.isFinite() || c.isFinite() || d.isFinite();
}

inline bool mostFinite(Arg a, Arg b, Arg c, Arg d, Arg e)
{
	return a.isFinite() || b.isFinite() || c.isFinite() || d.isFinite() || e.isFinite();
}

inline bool mostFinite(Arg a, Arg b, Arg c, Arg d, Arg e, Arg f, Arg g, Arg h)
{
	return a.isFinite() || b.isFinite() || c.isFinite() || d.isFinite() || e.isFinite() || f.isFinite() || g.isFinite() || h.isFinite();
}

inline bool leastFinite(Arg a, Arg b)
{
	return a.isFinite() && b.isFinite();
}

P<Form> asParent(Thread& th, V& v);

P<Form> consForm(P<Table> const& a, P<Form> const& d);
P<GForm> consForm(P<GTable> const& a, P<GForm> const& d);

void zprintf(std::string& out, const char* fmt, ...);

//==============================================================================
// ArgInfo - Argument information for each operations
//==============================================================================

struct ArgInfo
{
	int numArgs;
	struct {
		BothIn in;
		uint32_t mask;
	} arg[kMaxArgs];
};

List* handleEachOps(Thread& th, int numArgs, Arg fun);

P<Form> linearizeInheritance(Thread& th, size_t numArgs, V* args);

//==============================================================================
// RAII helper classes
//==============================================================================

#if defined(__APPLE__)
#include <CoreFoundation/CFBase.h>

class CFReleaser
{
	CFTypeRef p;
public:
	CFReleaser(CFTypeRef inP) : p(inP) {}
	~CFReleaser() { release(); }
	void release() { if (p) { CFRelease(p); p = nullptr; }}
};
#endif

class Freer
{
	void *p;
public:
	Freer(void* inP) : p(inP) {}
	~Freer() { dispose(); }
	void dispose() { if (p) { free(p); p = nullptr; } }
};

template <class T>
class Deleter
{
	T* p;
public:
	Deleter(T* inP) : p(inP) {}
	~Deleter() { delete p; }
};

template <class T>
class ArrayDeleter
{
	T* p;
public:
	ArrayDeleter(T* inP) : p(inP) {}
	~ArrayDeleter() { delete [] p; }
};

class ScopeLog
{
	const char* label;
public:
	ScopeLog(const char* inLabel) : label(inLabel) { post("%s {\n", label); }
	~ScopeLog() { post("} %s\n", label); }
};

//==============================================================================
// Include inline implementations that need complete types
// This MUST be last, after all class definitions
//==============================================================================

#include "ObjectInlines.hpp"

#endif // __Object_h__
