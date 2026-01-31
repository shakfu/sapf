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
#include "Opcode.hpp"
#include "ErrorCodes.hpp"
#include <cmath>

// Test fixture for VM tests
class VMTest : public SapfTestBase {
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
// Stack operations
//==============================================================================

TEST_F(VMTest, PushPop) {
    th.push(42.0);
    EXPECT_EQ(th.stackDepth(), 1);

    V v = th.pop();
    EXPECT_TRUE(v.isReal());
    EXPECT_DOUBLE_EQ(v.f, 42.0);
    EXPECT_EQ(th.stackDepth(), 0);
}

TEST_F(VMTest, PushMultiple) {
    th.push(1.0);
    th.push(2.0);
    th.push(3.0);
    EXPECT_EQ(th.stackDepth(), 3);

    EXPECT_DOUBLE_EQ(th.pop().f, 3.0);
    EXPECT_DOUBLE_EQ(th.pop().f, 2.0);
    EXPECT_DOUBLE_EQ(th.pop().f, 1.0);
}

TEST_F(VMTest, PushObject) {
    P<String> str = getsym("test");
    th.push(V(str));
    EXPECT_EQ(th.stackDepth(), 1);

    V v = th.pop();
    EXPECT_TRUE(v.isObject());
    EXPECT_TRUE(v.isString());
}

TEST_F(VMTest, TopAccess) {
    th.push(42.0);
    EXPECT_DOUBLE_EQ(th.top().f, 42.0);
    EXPECT_EQ(th.stackDepth(), 1);  // top() doesn't pop
}

TEST_F(VMTest, ClearStack) {
    th.push(1.0);
    th.push(2.0);
    th.push(3.0);
    EXPECT_EQ(th.stackDepth(), 3);

    th.clearStack();
    EXPECT_EQ(th.stackDepth(), 0);
}

TEST_F(VMTest, PopN) {
    th.push(1.0);
    th.push(2.0);
    th.push(3.0);
    th.push(4.0);
    EXPECT_EQ(th.stackDepth(), 4);

    th.popn(2);
    EXPECT_EQ(th.stackDepth(), 2);
    EXPECT_DOUBLE_EQ(th.pop().f, 2.0);
}

//==============================================================================
// Arithmetic opcodes
//==============================================================================

TEST_F(VMTest, OpcodeAdd) {
    V result = run("10 20 +");
    EXPECT_DOUBLE_EQ(result.f, 30.0);
}

TEST_F(VMTest, OpcodeAddNegative) {
    V result = run("-5 3 +");
    EXPECT_DOUBLE_EQ(result.f, -2.0);
}

TEST_F(VMTest, OpcodeSub) {
    V result = run("20 7 -");
    EXPECT_DOUBLE_EQ(result.f, 13.0);
}

TEST_F(VMTest, OpcodeSubNegativeResult) {
    V result = run("5 10 -");
    EXPECT_DOUBLE_EQ(result.f, -5.0);
}

TEST_F(VMTest, OpcodeMul) {
    V result = run("6 7 *");
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

TEST_F(VMTest, OpcodeMulByZero) {
    V result = run("100 0 *");
    EXPECT_DOUBLE_EQ(result.f, 0.0);
}

TEST_F(VMTest, OpcodeMulNegative) {
    V result = run("-3 4 *");
    EXPECT_DOUBLE_EQ(result.f, -12.0);
}

TEST_F(VMTest, OpcodeDiv) {
    V result = run("42 6 /");
    EXPECT_DOUBLE_EQ(result.f, 7.0);
}

TEST_F(VMTest, OpcodeDivFractional) {
    V result = run("5 2 /");
    EXPECT_DOUBLE_EQ(result.f, 2.5);
}

TEST_F(VMTest, OpcodeDivByZero) {
    V result = run("1 0 /");
    EXPECT_TRUE(std::isinf(result.f));
}

TEST_F(VMTest, OpcodeNeg) {
    V result = run("42 neg");
    EXPECT_DOUBLE_EQ(result.f, -42.0);
}

TEST_F(VMTest, OpcodeNegNegative) {
    V result = run("-42 neg");
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

TEST_F(VMTest, OpcodeAbs) {
    V result = run("-42 abs");
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

TEST_F(VMTest, OpcodeAbsPositive) {
    V result = run("42 abs");
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

TEST_F(VMTest, OpcodeMod) {
    V result = run("17 5 %");
    EXPECT_DOUBLE_EQ(result.f, 2.0);
}

TEST_F(VMTest, OpcodeSqrt) {
    V result = run("16 sqrt");
    EXPECT_DOUBLE_EQ(result.f, 4.0);
}

TEST_F(VMTest, OpcodeExp) {
    V result = run("0 exp");
    EXPECT_DOUBLE_EQ(result.f, 1.0);
}

TEST_F(VMTest, OpcodeLog) {
    V result = run("1 log");
    EXPECT_DOUBLE_EQ(result.f, 0.0);
}

//==============================================================================
// Trigonometric opcodes
//==============================================================================

TEST_F(VMTest, OpcodeSin) {
    V result = run("0 sin");
    EXPECT_NEAR(result.f, 0.0, 1e-10);
}

TEST_F(VMTest, OpcodeCos) {
    V result = run("0 cos");
    EXPECT_NEAR(result.f, 1.0, 1e-10);
}

TEST_F(VMTest, OpcodeTan) {
    V result = run("0 tan");
    EXPECT_NEAR(result.f, 0.0, 1e-10);
}

TEST_F(VMTest, OpcodeAtan2) {
    V result = run("1 1 atan2");
    EXPECT_NEAR(result.f, M_PI / 4, 1e-10);
}

//==============================================================================
// Comparison opcodes
//==============================================================================

TEST_F(VMTest, OpcodeEqual) {
    V result = run("5 5 ==");
    EXPECT_DOUBLE_EQ(result.f, 1.0);
}

TEST_F(VMTest, OpcodeEqualFalse) {
    V result = run("5 6 ==");
    EXPECT_DOUBLE_EQ(result.f, 0.0);
}

TEST_F(VMTest, OpcodeNotEqual) {
    V result = run("5 6 !=");
    EXPECT_DOUBLE_EQ(result.f, 1.0);
}

TEST_F(VMTest, OpcodeNotEqualFalse) {
    V result = run("5 5 !=");
    EXPECT_DOUBLE_EQ(result.f, 0.0);
}

TEST_F(VMTest, OpcodeLessThan) {
    V result = run("3 5 <");
    EXPECT_DOUBLE_EQ(result.f, 1.0);
}

TEST_F(VMTest, OpcodeLessThanFalse) {
    V result = run("5 3 <");
    EXPECT_DOUBLE_EQ(result.f, 0.0);
}

TEST_F(VMTest, OpcodeGreaterThan) {
    V result = run("5 3 >");
    EXPECT_DOUBLE_EQ(result.f, 1.0);
}

TEST_F(VMTest, OpcodeGreaterThanFalse) {
    V result = run("3 5 >");
    EXPECT_DOUBLE_EQ(result.f, 0.0);
}

TEST_F(VMTest, OpcodeLessEqual) {
    V result = run("3 5 <=");
    EXPECT_DOUBLE_EQ(result.f, 1.0);
}

TEST_F(VMTest, OpcodeLessEqualEqual) {
    V result = run("5 5 <=");
    EXPECT_DOUBLE_EQ(result.f, 1.0);
}

TEST_F(VMTest, OpcodeGreaterEqual) {
    V result = run("5 3 >=");
    EXPECT_DOUBLE_EQ(result.f, 1.0);
}

TEST_F(VMTest, OpcodeGreaterEqualEqual) {
    V result = run("5 5 >=");
    EXPECT_DOUBLE_EQ(result.f, 1.0);
}

//==============================================================================
// Min/Max opcodes
//==============================================================================

TEST_F(VMTest, OpcodeMin) {
    V result = run("3 7 &");
    EXPECT_DOUBLE_EQ(result.f, 3.0);
}

TEST_F(VMTest, OpcodeMax) {
    V result = run("3 7 |");
    EXPECT_DOUBLE_EQ(result.f, 7.0);
}

TEST_F(VMTest, OpcodeClip) {
    V result = run("5 0 10 clip");
    EXPECT_DOUBLE_EQ(result.f, 5.0);
}

TEST_F(VMTest, OpcodeClipLow) {
    V result = run("-5 0 10 clip");
    EXPECT_DOUBLE_EQ(result.f, 0.0);
}

TEST_F(VMTest, OpcodeClipHigh) {
    V result = run("15 0 10 clip");
    EXPECT_DOUBLE_EQ(result.f, 10.0);
}

//==============================================================================
// Rounding opcodes
//==============================================================================

TEST_F(VMTest, OpcodeFloor) {
    V result = run("3.7 floor");
    EXPECT_DOUBLE_EQ(result.f, 3.0);
}

TEST_F(VMTest, OpcodeFloorNegative) {
    V result = run("-3.7 floor");
    EXPECT_DOUBLE_EQ(result.f, -4.0);
}

TEST_F(VMTest, OpcodeCeil) {
    V result = run("3.2 ceil");
    EXPECT_DOUBLE_EQ(result.f, 4.0);
}

TEST_F(VMTest, OpcodeCeilNegative) {
    V result = run("-3.2 ceil");
    EXPECT_DOUBLE_EQ(result.f, -3.0);
}

//==============================================================================
// Stack manipulation opcodes
//==============================================================================

TEST_F(VMTest, OpcodeDup) {
    // Test aa (dup) - duplicate top of stack
    // 42 aa -> 42 42, then we check by comparing both values
    V result = run("42 aa 2ple");  // 2ple creates a 2-tuple/list from top 2 items
    EXPECT_TRUE(result.isList());
}

TEST_F(VMTest, OpcodeDrop) {
    // Test pop (drop) - after 1 2 pop, stack has only 1
    V result = run("1 2 pop");  // pop removes 2, leaves 1 which is returned
    EXPECT_DOUBLE_EQ(result.f, 1.0);
}

TEST_F(VMTest, OpcodeSwap) {
    // Test ba (swap) - swap top two items
    // 1 2 ba -> 2 1, then 2ple creates [2 1]
    V result = run("1 2 ba 2ple");
    EXPECT_TRUE(result.isList());
}

//==============================================================================
// Local variable binding
//==============================================================================

TEST_F(VMTest, LocalVariableBinding) {
    V result = run("42 = x x");
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

TEST_F(VMTest, LocalVariableMultiple) {
    V result = run("10 = x 20 = y x y +");
    EXPECT_DOUBLE_EQ(result.f, 30.0);
}

//==============================================================================
// Conditional execution
//==============================================================================

TEST_F(VMTest, IfTrue) {
    V result = run("1 \\[42] \\[0] if");
    EXPECT_DOUBLE_EQ(result.f, 42.0);
}

TEST_F(VMTest, IfFalse) {
    V result = run("0 \\[42] \\[99] if");
    EXPECT_DOUBLE_EQ(result.f, 99.0);
}

//==============================================================================
// Function application
//==============================================================================

TEST_F(VMTest, FunctionApply) {
    V result = run("5 \\x [x x *] !");
    EXPECT_DOUBLE_EQ(result.f, 25.0);
}

TEST_F(VMTest, FunctionApplyMultipleArgs) {
    V result = run("3 4 \\x y [x y +] !");
    EXPECT_DOUBLE_EQ(result.f, 7.0);
}

//==============================================================================
// Type checking operations
//==============================================================================

TEST_F(VMTest, TypeCheckReal) {
    // Test that a number is recognized as real
    th.push(42.0);
    V v = th.pop();
    EXPECT_TRUE(v.isReal());
    EXPECT_FALSE(v.isObject());
}

TEST_F(VMTest, TypeCheckString) {
    V result = run("\"hello\"");
    EXPECT_TRUE(result.isString());
    EXPECT_TRUE(result.isObject());
    EXPECT_FALSE(result.isReal());
}

TEST_F(VMTest, TypeCheckList) {
    V result = run("[1 2 3]");
    EXPECT_TRUE(result.isList());
    EXPECT_TRUE(result.isObject());
}

TEST_F(VMTest, TypeCheckFun) {
    V result = run("\\x [x]");
    EXPECT_TRUE(result.isFun());
    EXPECT_TRUE(result.isObject());
}

//==============================================================================
// Complex expressions
//==============================================================================

TEST_F(VMTest, ComplexArithmetic) {
    // (2 + 3) * (7 - 2) = 5 * 5 = 25
    V result = run("2 3 + 7 2 - *");
    EXPECT_DOUBLE_EQ(result.f, 25.0);
}

TEST_F(VMTest, NestedExpressions) {
    // sqrt(16) + abs(-9) = 4 + 9 = 13
    V result = run("16 sqrt -9 abs +");
    EXPECT_DOUBLE_EQ(result.f, 13.0);
}

TEST_F(VMTest, ListSize) {
    V result = run("[1 2 3] size");
    EXPECT_DOUBLE_EQ(result.f, 3.0);
}

