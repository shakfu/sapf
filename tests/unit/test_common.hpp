//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#ifndef SAPF_TEST_COMMON_HPP
#define SAPF_TEST_COMMON_HPP

#include "sapf/Engine.hpp"

// Initialize the SAPF engine once before any tests run.
// This registers all built-in operations (AddCoreOps, AddMathOps, etc.)
// which are required for parsing and executing SAPF code.
inline void initTestEngine() {
    static bool initialized = false;
    if (!initialized) {
        SapfEngine& engine = GetSapfEngine();
        engine.initialize();
        initialized = true;
    }
}

// Test fixture base class that ensures engine is initialized
class SapfTestBase : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        initTestEngine();
    }
};

#endif // SAPF_TEST_COMMON_HPP
