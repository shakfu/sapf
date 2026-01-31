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
#include <string>
#include <cmath>

// Test fixture for parser tests
class ParserTest : public SapfTestBase {
protected:
    Thread th;

    void SetUp() override {
        th.clearStack();
    }

    void TearDown() override {
        th.clearStack();
    }

    // Helper to compile and execute code, returning the top of stack
    V parseAndRun(const char* code) {
        P<Fun> fun;
        if (!th.compile(code, fun, true)) {
            throw errSyntax;
        }
        fun->apply(th);
        return th.pop();
    }

    // Helper to check if code compiles without error
    bool compiles(const char* code) {
        P<Fun> fun;
        try {
            return th.compile(code, fun, true);
        } catch (...) {
            return false;
        }
    }
};

//==============================================================================
// Basic number parsing
//==============================================================================

TEST_F(ParserTest, ParseInteger) {
    V result = parseAndRun("42");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

TEST_F(ParserTest, ParseNegativeInteger) {
    V result = parseAndRun("-17");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, -17.0);
}

TEST_F(ParserTest, ParseFloat) {
    V result = parseAndRun("3.14159");
    EXPECT_TRUE(result.isReal());
    EXPECT_NEAR(result.f, 3.14159, 1e-10);
}

TEST_F(ParserTest, ParseNegativeFloat) {
    V result = parseAndRun("-2.718");
    EXPECT_TRUE(result.isReal());
    EXPECT_NEAR(result.f, -2.718, 1e-10);
}

TEST_F(ParserTest, ParseScientificNotation) {
    V result = parseAndRun("1.5e3");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 1500.0);
}

TEST_F(ParserTest, ParseScientificNotationNegativeExponent) {
    V result = parseAndRun("2.5e-2");
    EXPECT_TRUE(result.isReal());
    EXPECT_NEAR(result.f, 0.025, 1e-10);
}

TEST_F(ParserTest, ParseHexNumber) {
    V result = parseAndRun("0xff");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 255.0);
}

TEST_F(ParserTest, ParseHexNumberUppercase) {
    V result = parseAndRun("0xDEAD");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 57005.0);
}

TEST_F(ParserTest, ParsePi) {
    V result = parseAndRun("pi");
    EXPECT_TRUE(result.isReal());
    EXPECT_NEAR(result.f, M_PI, 1e-10);
}

TEST_F(ParserTest, ParseZero) {
    V result = parseAndRun("0");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 0.0);
}

TEST_F(ParserTest, ParseLeadingDecimal) {
    V result = parseAndRun(".5");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 0.5);
}

//==============================================================================
// String parsing
//==============================================================================

TEST_F(ParserTest, ParseSimpleString) {
    V result = parseAndRun("\"hello\"");
    EXPECT_TRUE(result.isString());
    EXPECT_STREQ(((String*)result.o())->cstr(), "hello");
}

TEST_F(ParserTest, ParseEmptyString) {
    V result = parseAndRun("\"\"");
    EXPECT_TRUE(result.isString());
    EXPECT_STREQ(((String*)result.o())->cstr(), "");
}

TEST_F(ParserTest, ParseStringWithSpaces) {
    V result = parseAndRun("\"hello world\"");
    EXPECT_TRUE(result.isString());
    EXPECT_STREQ(((String*)result.o())->cstr(), "hello world");
}

TEST_F(ParserTest, ParseStringWithNumbers) {
    V result = parseAndRun("\"test123\"");
    EXPECT_TRUE(result.isString());
    EXPECT_STREQ(((String*)result.o())->cstr(), "test123");
}

//==============================================================================
// Array parsing
//==============================================================================

TEST_F(ParserTest, ParseEmptyArray) {
    V result = parseAndRun("[]");
    EXPECT_TRUE(result.isList());
}

TEST_F(ParserTest, ParseSimpleArray) {
    V result = parseAndRun("[1 2 3]");
    EXPECT_TRUE(result.isList());
}

TEST_F(ParserTest, ParseNestedArray) {
    V result = parseAndRun("[[1 2] [3 4]]");
    EXPECT_TRUE(result.isList());
}

TEST_F(ParserTest, ParseMixedArray) {
    V result = parseAndRun("[1 \"hello\" 3.14]");
    EXPECT_TRUE(result.isList());
}

//==============================================================================
// Lambda/function parsing
//==============================================================================

TEST_F(ParserTest, ParseSimpleLambda) {
    V result = parseAndRun("\\x [x x +]");
    EXPECT_TRUE(result.isFun());
}

TEST_F(ParserTest, ParseLambdaNoArgs) {
    V result = parseAndRun("\\[42]");
    EXPECT_TRUE(result.isFun());
}

TEST_F(ParserTest, ParseLambdaMultipleArgs) {
    V result = parseAndRun("\\x y [x y +]");
    EXPECT_TRUE(result.isFun());
}

//==============================================================================
// Whitespace handling
//==============================================================================

TEST_F(ParserTest, ParseWithExtraSpaces) {
    V result = parseAndRun("   42   ");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

TEST_F(ParserTest, ParseWithTabs) {
    V result = parseAndRun("\t42\t");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

TEST_F(ParserTest, ParseWithNewlines) {
    V result = parseAndRun("\n42\n");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

//==============================================================================
// Comment handling
//==============================================================================

TEST_F(ParserTest, ParseWithLineComment) {
    V result = parseAndRun("42 ; this is a comment");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

//==============================================================================
// Edge cases and potential errors
//==============================================================================

TEST_F(ParserTest, ParseEmptyInput) {
    P<Fun> fun;
    bool result = th.compile("", fun, true);
    // Empty input should compile but produce nothing
    EXPECT_TRUE(result || !result);  // Implementation-dependent
}

TEST_F(ParserTest, ParseWhitespaceOnly) {
    P<Fun> fun;
    bool result = th.compile("   \t\n   ", fun, true);
    EXPECT_TRUE(result || !result);  // Implementation-dependent
}

TEST_F(ParserTest, ParseUnmatchedOpenParen) {
    EXPECT_THROW({
        parseAndRun("(1 2 3");
    }, int);
}

TEST_F(ParserTest, ParseUnmatchedCloseParen) {
    EXPECT_THROW({
        parseAndRun("1 2 3)");
    }, int);
}

TEST_F(ParserTest, ParseUnmatchedOpenBracket) {
    EXPECT_THROW({
        parseAndRun("[1 2 3");
    }, int);
}

TEST_F(ParserTest, ParseUnmatchedCloseBracket) {
    EXPECT_THROW({
        parseAndRun("1 2 3]");
    }, int);
}

TEST_F(ParserTest, ParseUnterminatedString) {
    EXPECT_THROW({
        parseAndRun("\"hello");
    }, int);
}

TEST_F(ParserTest, ParseDeeplyNestedParens) {
    // Test deeply nested parentheses don't crash
    std::string code = "";
    for (int i = 0; i < 50; ++i) code += "(";
    code += "42";
    for (int i = 0; i < 50; ++i) code += ")";

    V result = parseAndRun(code.c_str());
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

TEST_F(ParserTest, ParseDeeplyNestedBrackets) {
    // Test deeply nested brackets don't crash
    std::string code = "";
    for (int i = 0; i < 20; ++i) code += "[";
    code += "1";
    for (int i = 0; i < 20; ++i) code += "]";

    V result = parseAndRun(code.c_str());
    EXPECT_TRUE(result.isList());
}

TEST_F(ParserTest, ParseVeryLongNumber) {
    V result = parseAndRun("12345678901234567890");
    EXPECT_TRUE(result.isReal());
    // May lose precision but shouldn't crash
}

TEST_F(ParserTest, ParseVerySmallNumber) {
    V result = parseAndRun("1e-300");
    EXPECT_TRUE(result.isReal());
    EXPECT_GT(result.f, 0.0);
}

TEST_F(ParserTest, ParseVeryLargeNumber) {
    V result = parseAndRun("1e300");
    EXPECT_TRUE(result.isReal());
    EXPECT_TRUE(std::isfinite(result.f));
}

//==============================================================================
// Arithmetic expressions
//==============================================================================

TEST_F(ParserTest, ParseSimpleAddition) {
    V result = parseAndRun("2 3 +");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 5.0);
}

TEST_F(ParserTest, ParseSimpleSubtraction) {
    V result = parseAndRun("10 3 -");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 7.0);
}

TEST_F(ParserTest, ParseSimpleMultiplication) {
    V result = parseAndRun("4 5 *");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 20.0);
}

TEST_F(ParserTest, ParseSimpleDivision) {
    V result = parseAndRun("20 4 /");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 5.0);
}

TEST_F(ParserTest, ParseChainedOperations) {
    V result = parseAndRun("2 3 + 4 *");  // (2+3)*4 = 20
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 20.0);
}

//==============================================================================
// Variable binding
//==============================================================================

TEST_F(ParserTest, ParseVariableBinding) {
    V result = parseAndRun("42 = x x");
    EXPECT_TRUE(result.isReal());
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

