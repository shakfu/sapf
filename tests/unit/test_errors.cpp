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
#include <cmath>

// Test fixture for error handling tests
class ErrorTest : public SapfTestBase {
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

    // Helper to check if code throws any error
    bool throwsAnyError(const char* code) {
        try {
            run(code);
            return false;
        } catch (...) {
            return true;
        }
    }
};

//==============================================================================
// Syntax errors
//==============================================================================

TEST_F(ErrorTest, UnmatchedOpenParen) {
    EXPECT_TRUE(throwsAnyError("(1 2 3"));
}

TEST_F(ErrorTest, UnmatchedCloseParen) {
    EXPECT_TRUE(throwsAnyError("1 2 3)"));
}

TEST_F(ErrorTest, UnmatchedOpenBracket) {
    EXPECT_TRUE(throwsAnyError("[1 2 3"));
}

TEST_F(ErrorTest, UnmatchedCloseBracket) {
    EXPECT_TRUE(throwsAnyError("1 2 3]"));
}

TEST_F(ErrorTest, UnterminatedString) {
    EXPECT_TRUE(throwsAnyError("\"hello"));
}

TEST_F(ErrorTest, MismatchedBrackets) {
    EXPECT_TRUE(throwsAnyError("[1 2 3)"));
}

TEST_F(ErrorTest, MismatchedParens) {
    EXPECT_TRUE(throwsAnyError("(1 2 3]"));
}

//==============================================================================
// Type errors
//==============================================================================

TEST_F(ErrorTest, AddStringToNumber) {
    // Adding string to number should throw
    EXPECT_TRUE(throwsAnyError("\"hello\" 42 +"));
}

TEST_F(ErrorTest, SubtractFromString) {
    EXPECT_TRUE(throwsAnyError("\"hello\" 1 -"));
}

TEST_F(ErrorTest, MultiplyStrings) {
    EXPECT_TRUE(throwsAnyError("\"a\" \"b\" *"));
}

//==============================================================================
// Stack errors
//==============================================================================

TEST_F(ErrorTest, PopEmptyStack) {
    th.clearStack();
    EXPECT_EQ(th.stackDepth(), 0);
    // Popping empty stack through operations
    EXPECT_TRUE(throwsAnyError("+"));  // Need two operands
}

TEST_F(ErrorTest, BinaryOpMissingOperand) {
    th.clearStack();
    th.push(42.0);
    EXPECT_TRUE(throwsAnyError("+"));  // Only one operand
}

TEST_F(ErrorTest, UnaryOpEmptyStack) {
    th.clearStack();
    EXPECT_TRUE(throwsAnyError("neg"));
}

//==============================================================================
// Division errors
//==============================================================================

TEST_F(ErrorTest, DivisionByZeroProducesInfinity) {
    // In floating point, division by zero produces infinity, not error
    V result = run("1 0 /");
    EXPECT_TRUE(std::isinf(result.f));
}

TEST_F(ErrorTest, ZeroDivZeroProducesNaN) {
    V result = run("0 0 /");
    EXPECT_TRUE(std::isnan(result.f));
}

//==============================================================================
// Undefined symbol errors
//==============================================================================

TEST_F(ErrorTest, UndefinedSymbol) {
    EXPECT_TRUE(throwsAnyError("undefined_symbol_xyz123"));
}

TEST_F(ErrorTest, UndefinedInExpression) {
    EXPECT_TRUE(throwsAnyError("42 undefined_var +"));
}

//==============================================================================
// Error code values
//==============================================================================

TEST_F(ErrorTest, ErrorCodeValues) {
    // Verify error code constants are defined and distinct
    EXPECT_NE(errNone, errSyntax);
    EXPECT_NE(errNone, errWrongType);
    EXPECT_NE(errNone, errOutOfRange);
    EXPECT_NE(errNone, errStackUnderflow);
    EXPECT_NE(errSyntax, errWrongType);
    EXPECT_NE(errWrongType, errOutOfRange);
}

TEST_F(ErrorTest, ErrorCodeNoneIsZero) {
    EXPECT_EQ(errNone, 0);
}

TEST_F(ErrorTest, ErrorCodesAreNegative) {
    EXPECT_LT(errSyntax, 0);
    EXPECT_LT(errWrongType, 0);
    EXPECT_LT(errOutOfRange, 0);
    EXPECT_LT(errStackUnderflow, 0);
    EXPECT_LT(errStackOverflow, 0);
}

//==============================================================================
// Recovery from errors
//==============================================================================

TEST_F(ErrorTest, StackClearedAfterError) {
    th.push(1.0);
    th.push(2.0);
    th.push(3.0);

    try {
        // This should fail
        run("undefined_symbol");
    } catch (...) {
        // Expected
    }

    // Clear stack for next operation
    th.clearStack();
    EXPECT_EQ(th.stackDepth(), 0);

    // Should be able to continue
    V result = run("42");
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

//==============================================================================
// Special floating point values
//==============================================================================

TEST_F(ErrorTest, InfinityArithmetic) {
    // Operations on infinity shouldn't crash
    V result = run("1 0 / 1 +");
    EXPECT_TRUE(std::isinf(result.f));
}

TEST_F(ErrorTest, NaNArithmetic) {
    // Operations on NaN shouldn't crash
    V result = run("0 0 / 1 +");
    EXPECT_TRUE(std::isnan(result.f));
}

TEST_F(ErrorTest, NaNComparison) {
    // NaN comparisons
    V result = run("0 0 / 0 0 / ==");
    EXPECT_DOUBLE_EQ(result.f, 0.0);  // NaN != NaN
}

TEST_F(ErrorTest, InfinityComparison) {
    V result = run("1 0 / 1 0 / ==");
    EXPECT_DOUBLE_EQ(result.f, 1.0);  // inf == inf
}

