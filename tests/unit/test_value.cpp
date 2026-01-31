//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include <gtest/gtest.h>
#include "VM.hpp"  // Needed for RCObj::retain/release inline definitions
#include <cmath>
#include <limits>

// Test fixture for V (Value) class tests
class ValueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Nothing to set up
    }

    void TearDown() override {
        // Nothing to tear down
    }
};

//==============================================================================
// Construction tests
//==============================================================================

TEST_F(ValueTest, DefaultConstructorCreatesZero) {
    V v;
    EXPECT_TRUE(v.isReal());
    EXPECT_FALSE(v.isObject());
    EXPECT_DOUBLE_EQ(v.f, 0.0);
}

TEST_F(ValueTest, DoubleConstructor) {
    V v(3.14159);
    EXPECT_TRUE(v.isReal());
    EXPECT_FALSE(v.isObject());
    EXPECT_DOUBLE_EQ(v.f, 3.14159);
}

TEST_F(ValueTest, NegativeDouble) {
    V v(-42.5);
    EXPECT_TRUE(v.isReal());
    EXPECT_DOUBLE_EQ(v.f, -42.5);
}

TEST_F(ValueTest, ObjectConstructor) {
    P<String> str = new String("test");
    V v(str);
    EXPECT_TRUE(v.isObject());
    EXPECT_FALSE(v.isReal());
    EXPECT_EQ(v.o(), str());
}

TEST_F(ValueTest, ObjectPointerConstructor) {
    String* str = new String("test");
    V v(str);
    EXPECT_TRUE(v.isObject());
    EXPECT_EQ(v.o(), str);
}

//==============================================================================
// Type checking tests
//==============================================================================

TEST_F(ValueTest, IsZeroForZeroValue) {
    V zero(0.0);
    EXPECT_TRUE(zero.isZero());
    EXPECT_TRUE(zero.isReal());
}

TEST_F(ValueTest, IsZeroFalseForNonZero) {
    V nonzero(1.0);
    EXPECT_FALSE(nonzero.isZero());
}

TEST_F(ValueTest, IsZeroFalseForObject) {
    P<String> str = new String("test");
    V v(str);
    EXPECT_FALSE(v.isZero());
}

TEST_F(ValueTest, IsStringForStringObject) {
    P<String> str = new String("hello");
    V v(str);
    EXPECT_TRUE(v.isString());
}

TEST_F(ValueTest, IsStringFalseForReal) {
    V v(42.0);
    EXPECT_FALSE(v.isString());
}

//==============================================================================
// Type name tests
//==============================================================================

TEST_F(ValueTest, TypeNameReal) {
    V v(42.0);
    EXPECT_STREQ(v.TypeName(), "Real");
}

TEST_F(ValueTest, TypeNameString) {
    P<String> str = new String("test");
    V v(str);
    EXPECT_STREQ(v.TypeName(), "String");
}

//==============================================================================
// Boolean conversion tests
//==============================================================================

TEST_F(ValueTest, IsTrueForNonZeroReal) {
    V v(1.0);
    EXPECT_TRUE(v.isTrue());
    EXPECT_FALSE(v.isFalse());
}

TEST_F(ValueTest, IsFalseForZeroReal) {
    V v(0.0);
    EXPECT_FALSE(v.isTrue());
    EXPECT_TRUE(v.isFalse());
}

TEST_F(ValueTest, IsTrueForNegativeReal) {
    V v(-1.0);
    EXPECT_TRUE(v.isTrue());
}

TEST_F(ValueTest, IsTrueForObject) {
    P<String> str = new String("test");
    V v(str);
    EXPECT_TRUE(v.isTrue());  // Objects are truthy
}

//==============================================================================
// asFloat / asInt tests
//==============================================================================

TEST_F(ValueTest, AsFloatFromReal) {
    V v(42.5);
    EXPECT_DOUBLE_EQ(v.asFloat(), 42.5);
}

TEST_F(ValueTest, AsIntFromReal) {
    V v(42.7);
    EXPECT_EQ(v.asInt(), 43);  // Rounds to nearest
}

TEST_F(ValueTest, AsIntRoundsCorrectly) {
    V v1(42.4);
    V v2(42.5);
    V v3(42.6);

    EXPECT_EQ(v1.asInt(), 42);
    EXPECT_EQ(v2.asInt(), 43);  // 0.5 rounds up
    EXPECT_EQ(v3.asInt(), 43);
}

TEST_F(ValueTest, AsIntNegativeRounding) {
    V v1(-42.4);
    V v2(-42.5);
    V v3(-42.6);

    EXPECT_EQ(v1.asInt(), -42);
    EXPECT_EQ(v2.asInt(), -42);  // floor(-42 + 0.5) = floor(-41.5) = -42
    EXPECT_EQ(v3.asInt(), -43);
}

//==============================================================================
// Set method tests
//==============================================================================

TEST_F(ValueTest, SetDouble) {
    V v(42.0);
    v.set(100.0);
    EXPECT_TRUE(v.isReal());
    EXPECT_DOUBLE_EQ(v.f, 100.0);
}

TEST_F(ValueTest, SetObject) {
    V v(42.0);
    P<String> str = new String("test");
    v.set(str());
    EXPECT_TRUE(v.isObject());
    EXPECT_EQ(v.o(), str());
}

TEST_F(ValueTest, SetFromValue) {
    V v1(42.0);
    V v2(100.0);
    v1.set(v2);
    EXPECT_DOUBLE_EQ(v1.f, 100.0);
}

//==============================================================================
// Special floating point values
//==============================================================================

TEST_F(ValueTest, InfinityValue) {
    V v(std::numeric_limits<double>::infinity());
    EXPECT_TRUE(v.isReal());
    EXPECT_TRUE(std::isinf(v.f));
}

TEST_F(ValueTest, NegativeInfinityValue) {
    V v(-std::numeric_limits<double>::infinity());
    EXPECT_TRUE(v.isReal());
    EXPECT_TRUE(std::isinf(v.f));
    EXPECT_TRUE(v.f < 0);
}

TEST_F(ValueTest, NaNValue) {
    V v(std::numeric_limits<double>::quiet_NaN());
    EXPECT_TRUE(v.isReal());
    EXPECT_TRUE(std::isnan(v.f));
}

TEST_F(ValueTest, NaNIsFalse) {
    // NaN == 0 is false, so isTrue should return true
    V v(std::numeric_limits<double>::quiet_NaN());
    EXPECT_TRUE(v.isTrue());  // !(NaN == 0) is true
}

//==============================================================================
// Identity tests
//==============================================================================

TEST_F(ValueTest, IdenticalReals) {
    V v1(42.0);
    V v2(42.0);
    EXPECT_TRUE(v1.Identical(v2));
}

TEST_F(ValueTest, NonIdenticalReals) {
    V v1(42.0);
    V v2(43.0);
    EXPECT_FALSE(v1.Identical(v2));
}

TEST_F(ValueTest, IdenticalObjects) {
    P<String> str = new String("test");
    V v1(str);
    V v2(str);
    EXPECT_TRUE(v1.Identical(v2));
}

TEST_F(ValueTest, NonIdenticalObjects) {
    P<String> str1 = new String("test");
    P<String> str2 = new String("test");  // Same content, different object
    V v1(str1);
    V v2(str2);
    EXPECT_FALSE(v1.Identical(v2));  // Different pointers
}

TEST_F(ValueTest, RealNotIdenticalToObject) {
    P<String> str = new String("42");
    V v1(42.0);
    V v2(str);
    EXPECT_FALSE(v1.Identical(v2));
    EXPECT_FALSE(v2.Identical(v1));
}

//==============================================================================
// Reference counting integration
//==============================================================================

TEST_F(ValueTest, ObjectRefcountOnConstruction) {
    P<String> str = new String("test");
    EXPECT_EQ(str->getRefcount(), 1);

    V v(str);
    EXPECT_EQ(str->getRefcount(), 2);  // P<> + V
}

TEST_F(ValueTest, ObjectRefcountOnCopy) {
    P<String> str = new String("test");
    V v1(str);
    V v2 = v1;
    EXPECT_EQ(str->getRefcount(), 3);  // P<> + v1 + v2
}

TEST_F(ValueTest, ObjectReleasedOnValueDestruction) {
    P<String> str = new String("test");
    EXPECT_EQ(str->getRefcount(), 1);

    {
        V v(str);
        EXPECT_EQ(str->getRefcount(), 2);
    }

    EXPECT_EQ(str->getRefcount(), 1);
}
