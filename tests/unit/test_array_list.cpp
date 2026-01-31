//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include <gtest/gtest.h>
#include "test_common.hpp"
#include "VM.hpp"
#include "ErrorCodes.hpp"

// Test fixture for Array and List tests
class ArrayListTest : public SapfTestBase {
protected:
    Thread th;

    void SetUp() override {
        th.clearStack();
    }

    void TearDown() override {
        th.clearStack();
    }

    // Helper to compile and execute code
    V run(const char* code) {
        P<Fun> fun;
        if (!th.compile(code, fun, true)) {
            throw errSyntax;
        }
        fun->apply(th);
        return th.pop();
    }
};

//==============================================================================
// Array construction
//==============================================================================

TEST_F(ArrayListTest, CreateEmptyVArray) {
    P<Array> arr = new Array(itemTypeV, 0);
    EXPECT_EQ(arr->size(), 0);
    EXPECT_TRUE(arr->isV());
    EXPECT_FALSE(arr->isZ());
}

TEST_F(ArrayListTest, CreateEmptyZArray) {
    P<Array> arr = new Array(itemTypeZ, 0);
    EXPECT_EQ(arr->size(), 0);
    EXPECT_FALSE(arr->isV());
    EXPECT_TRUE(arr->isZ());
}

TEST_F(ArrayListTest, CreateVArrayWithCapacity) {
    P<Array> arr = new Array(itemTypeV, 100);
    EXPECT_EQ(arr->size(), 0);  // Size is 0, capacity is 100
}

TEST_F(ArrayListTest, CreateZArrayWithCapacity) {
    P<Array> arr = new Array(itemTypeZ, 100);
    EXPECT_EQ(arr->size(), 0);
}

//==============================================================================
// Array add/put operations
//==============================================================================

TEST_F(ArrayListTest, ArrayAddV) {
    P<Array> arr = new Array(itemTypeV, 10);
    arr->add(V(42.0));
    EXPECT_EQ(arr->size(), 1);
    EXPECT_DOUBLE_EQ(arr->at(0).f, 42.0);
}

TEST_F(ArrayListTest, ArrayAddMultipleV) {
    P<Array> arr = new Array(itemTypeV, 10);
    arr->add(V(1.0));
    arr->add(V(2.0));
    arr->add(V(3.0));
    EXPECT_EQ(arr->size(), 3);
    EXPECT_DOUBLE_EQ(arr->at(0).f, 1.0);
    EXPECT_DOUBLE_EQ(arr->at(1).f, 2.0);
    EXPECT_DOUBLE_EQ(arr->at(2).f, 3.0);
}

TEST_F(ArrayListTest, ArrayAddZ) {
    P<Array> arr = new Array(itemTypeZ, 10);
    arr->addz(1.5);
    arr->addz(2.5);
    EXPECT_EQ(arr->size(), 2);
    EXPECT_DOUBLE_EQ(arr->atz(0), 1.5);
    EXPECT_DOUBLE_EQ(arr->atz(1), 2.5);
}

TEST_F(ArrayListTest, ArrayPutV) {
    P<Array> arr = new Array(itemTypeV, 10);
    arr->add(V(0.0));
    arr->add(V(0.0));
    arr->put(0, V(42.0));
    arr->put(1, V(99.0));
    EXPECT_DOUBLE_EQ(arr->at(0).f, 42.0);
    EXPECT_DOUBLE_EQ(arr->at(1).f, 99.0);
}

TEST_F(ArrayListTest, ArrayPutZ) {
    P<Array> arr = new Array(itemTypeZ, 10);
    arr->addz(0.0);
    arr->addz(0.0);
    arr->putz(0, 42.0);
    arr->putz(1, 99.0);
    EXPECT_DOUBLE_EQ(arr->atz(0), 42.0);
    EXPECT_DOUBLE_EQ(arr->atz(1), 99.0);
}

//==============================================================================
// Array indexing modes
//==============================================================================

TEST_F(ArrayListTest, ArrayAtBasic) {
    P<Array> arr = new Array(itemTypeV, 10);
    for (int i = 0; i < 5; i++) {
        arr->add(V((double)i * 10));
    }
    EXPECT_DOUBLE_EQ(arr->at(0).f, 0.0);
    EXPECT_DOUBLE_EQ(arr->at(2).f, 20.0);
    EXPECT_DOUBLE_EQ(arr->at(4).f, 40.0);
}

TEST_F(ArrayListTest, ArrayWrapAt) {
    P<Array> arr = new Array(itemTypeV, 10);
    arr->add(V(10.0));
    arr->add(V(20.0));
    arr->add(V(30.0));

    // Wrap around positive indices
    EXPECT_DOUBLE_EQ(arr->wrapAt(0).f, 10.0);
    EXPECT_DOUBLE_EQ(arr->wrapAt(3).f, 10.0);  // wraps
    EXPECT_DOUBLE_EQ(arr->wrapAt(4).f, 20.0);  // wraps
    EXPECT_DOUBLE_EQ(arr->wrapAt(5).f, 30.0);  // wraps
}

TEST_F(ArrayListTest, ArrayClipAt) {
    P<Array> arr = new Array(itemTypeV, 10);
    arr->add(V(10.0));
    arr->add(V(20.0));
    arr->add(V(30.0));

    // Clip to valid range
    EXPECT_DOUBLE_EQ(arr->clipAt(0).f, 10.0);
    EXPECT_DOUBLE_EQ(arr->clipAt(1).f, 20.0);
    EXPECT_DOUBLE_EQ(arr->clipAt(2).f, 30.0);
    EXPECT_DOUBLE_EQ(arr->clipAt(100).f, 30.0);  // clips to last
}

TEST_F(ArrayListTest, ArrayFoldAt) {
    P<Array> arr = new Array(itemTypeV, 10);
    arr->add(V(10.0));
    arr->add(V(20.0));
    arr->add(V(30.0));

    // Fold (reflect) at boundaries
    EXPECT_DOUBLE_EQ(arr->foldAt(0).f, 10.0);
    EXPECT_DOUBLE_EQ(arr->foldAt(1).f, 20.0);
    EXPECT_DOUBLE_EQ(arr->foldAt(2).f, 30.0);
    EXPECT_DOUBLE_EQ(arr->foldAt(3).f, 20.0);  // folds back
    EXPECT_DOUBLE_EQ(arr->foldAt(4).f, 10.0);  // folds back
}

//==============================================================================
// List construction via parsing
//==============================================================================

TEST_F(ArrayListTest, ParseEmptyList) {
    V result = run("[]");
    EXPECT_TRUE(result.isList());
}

TEST_F(ArrayListTest, ParseVList) {
    V result = run("[1 2 3]");
    EXPECT_TRUE(result.isList());
}

TEST_F(ArrayListTest, ParseNestedList) {
    V result = run("[[1 2] [3 4]]");
    EXPECT_TRUE(result.isList());
}

TEST_F(ArrayListTest, ListSize) {
    V result = run("[1 2 3 4 5] size");
    EXPECT_DOUBLE_EQ(result.f, 5.0);
}

//==============================================================================
// List operations via SAPF syntax
//==============================================================================

TEST_F(ArrayListTest, ListAt) {
    V result = run("[10 20 30] 1 at");
    EXPECT_DOUBLE_EQ(result.f, 20.0);
}

TEST_F(ArrayListTest, ListAtFirst) {
    V result = run("[10 20 30] 0 at");
    EXPECT_DOUBLE_EQ(result.f, 10.0);
}

TEST_F(ArrayListTest, ListAtLast) {
    V result = run("[10 20 30] 2 at");
    EXPECT_DOUBLE_EQ(result.f, 30.0);
}

TEST_F(ArrayListTest, ListReverse) {
    V result = run("[1 2 3] reverse");
    EXPECT_TRUE(result.isList());
}

//==============================================================================
// List transformations
//==============================================================================

TEST_F(ArrayListTest, ListFold) {
    V result = run("[1 2 3 4] +/");
    EXPECT_DOUBLE_EQ(result.f, 10.0);  // 1+2+3+4
}

TEST_F(ArrayListTest, ListScan) {
    V result = run("[1 2 3 4] +\\");
    EXPECT_TRUE(result.isList());
}

//==============================================================================
// List concatenation
//==============================================================================

TEST_F(ArrayListTest, ListConcat) {
    V result = run("[1 2] [3 4] $");
    EXPECT_TRUE(result.isList());
}

//==============================================================================
// Array/List type properties
//==============================================================================

TEST_F(ArrayListTest, ArrayIsArray) {
    P<Array> arr = new Array(itemTypeV, 10);
    EXPECT_TRUE(arr->isArray());
    EXPECT_FALSE(arr->isList());
    EXPECT_FALSE(arr->isString());
}

TEST_F(ArrayListTest, ListIsList) {
    V result = run("[1 2 3]");
    EXPECT_TRUE(result.o->isList());
    EXPECT_FALSE(result.o->isArray());
    EXPECT_FALSE(result.o->isString());
}

TEST_F(ArrayListTest, ArrayItemType) {
    P<Array> vArr = new Array(itemTypeV, 10);
    P<Array> zArr = new Array(itemTypeZ, 10);

    EXPECT_EQ(vArr->ItemType(), itemTypeV);
    EXPECT_EQ(zArr->ItemType(), itemTypeZ);
}

//==============================================================================
// Edge cases
//==============================================================================

TEST_F(ArrayListTest, SingleElementList) {
    V result = run("[42]");
    EXPECT_TRUE(result.isList());
}

TEST_F(ArrayListTest, ArrayGrowth) {
    P<Array> arr = new Array(itemTypeV, 1);  // Start small
    for (int i = 0; i < 100; i++) {
        arr->add(V((double)i));
    }
    EXPECT_EQ(arr->size(), 100);
    EXPECT_DOUBLE_EQ(arr->at(99).f, 99.0);
}

