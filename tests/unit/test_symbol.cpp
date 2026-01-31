//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include <gtest/gtest.h>
#include "VM.hpp"  // Needed for RCObj::retain/release inline definitions
#include "symbol.hpp"
#include <thread>
#include <vector>
#include <set>
#include <atomic>

// Test fixture for symbol table tests
class SymbolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Nothing to set up - symbol table is global
    }

    void TearDown() override {
        // Nothing to tear down - symbols are interned permanently
    }
};

//==============================================================================
// Basic symbol interning tests
//==============================================================================

TEST_F(SymbolTest, GetSymReturnsValidString) {
    P<String> sym = getsym("test_symbol_1");
    EXPECT_NE(sym(), nullptr);
    EXPECT_STREQ(sym->cstr(), "test_symbol_1");
}

TEST_F(SymbolTest, SameSymbolReturnsSamePointer) {
    P<String> sym1 = getsym("interned_symbol");
    P<String> sym2 = getsym("interned_symbol");

    EXPECT_EQ(sym1(), sym2());  // Same pointer
}

TEST_F(SymbolTest, DifferentSymbolsReturnDifferentPointers) {
    P<String> sym1 = getsym("symbol_a");
    P<String> sym2 = getsym("symbol_b");

    EXPECT_NE(sym1(), sym2());
}

TEST_F(SymbolTest, SymbolHashIsConsistent) {
    P<String> sym1 = getsym("hash_test");
    P<String> sym2 = getsym("hash_test");

    EXPECT_EQ(sym1->Hash(), sym2->Hash());
}

TEST_F(SymbolTest, EmptyStringSymbol) {
    P<String> sym = getsym("");
    EXPECT_NE(sym(), nullptr);
    EXPECT_STREQ(sym->cstr(), "");
}

TEST_F(SymbolTest, SymbolWithSpaces) {
    P<String> sym = getsym("symbol with spaces");
    EXPECT_NE(sym(), nullptr);
    EXPECT_STREQ(sym->cstr(), "symbol with spaces");
}

TEST_F(SymbolTest, SymbolWithSpecialChars) {
    P<String> sym = getsym("!@#$%^&*()");
    EXPECT_NE(sym(), nullptr);
    EXPECT_STREQ(sym->cstr(), "!@#$%^&*()");
}

TEST_F(SymbolTest, LongSymbol) {
    std::string longName(1000, 'x');
    P<String> sym = getsym(longName.c_str());
    EXPECT_NE(sym(), nullptr);
    EXPECT_EQ(sym->cstr(), longName);
}

//==============================================================================
// String class tests
//==============================================================================

TEST_F(SymbolTest, StringEquality) {
    P<String> sym1 = getsym("equality_test");
    P<String> sym2 = getsym("equality_test");

    // Create a mock thread for Equals (not actually used for strings)
    // Note: String::Equals checks pointer equality first, then hash+strcmp
    EXPECT_TRUE(sym1->hash == sym2->hash);
}

TEST_F(SymbolTest, StringComparison) {
    P<String> a = getsym("aaa");
    P<String> b = getsym("bbb");
    P<String> a2 = getsym("aaa");

    // String comparison uses strcmp
    // Note: Compare requires Thread&, so we just test the hash
    EXPECT_EQ(a->Hash(), a2->Hash());
    EXPECT_NE(a->Hash(), b->Hash());  // Different strings usually have different hashes
}

TEST_F(SymbolTest, StringLength) {
    P<String> sym = getsym("hello");
    // Note: String::length requires Thread&, but we can check cstr directly
    EXPECT_EQ(strlen(sym->cstr()), 5);
}

TEST_F(SymbolTest, StringIsString) {
    P<String> sym = getsym("type_test");
    EXPECT_TRUE(sym->isString());
    EXPECT_FALSE(sym->isArray());
    EXPECT_FALSE(sym->isList());
}

//==============================================================================
// Concurrent access tests
//==============================================================================

TEST_F(SymbolTest, ConcurrentSymbolCreation) {
    const int numThreads = 8;
    const int numSymbolsPerThread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Create unique symbols from multiple threads
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([t, &successCount, numSymbolsPerThread]() {
            for (int i = 0; i < numSymbolsPerThread; ++i) {
                std::string name = "concurrent_" + std::to_string(t) + "_" + std::to_string(i);
                P<String> sym = getsym(name.c_str());
                if (sym() != nullptr && sym->cstr() == name) {
                    successCount++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(successCount.load(), numThreads * numSymbolsPerThread);
}

TEST_F(SymbolTest, ConcurrentSameSymbol) {
    const int numThreads = 8;
    const int iterations = 100;
    std::vector<std::thread> threads;
    std::vector<String*> results(numThreads * iterations, nullptr);
    std::atomic<int> index{0};

    // All threads try to get the same symbol
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&results, &index, iterations]() {
            for (int i = 0; i < iterations; ++i) {
                P<String> sym = getsym("shared_symbol");
                int idx = index++;
                results[idx] = sym();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All results should be the same pointer
    String* first = results[0];
    EXPECT_NE(first, nullptr);
    for (const auto& ptr : results) {
        EXPECT_EQ(ptr, first);
    }
}

//==============================================================================
// Hash collision handling (probabilistic test)
//==============================================================================

TEST_F(SymbolTest, ManySymbolsNoCrash) {
    // Create many symbols to exercise hash table
    const int numSymbols = 10000;
    std::set<String*> uniquePointers;

    for (int i = 0; i < numSymbols; ++i) {
        std::string name = "mass_symbol_" + std::to_string(i);
        P<String> sym = getsym(name.c_str());
        EXPECT_NE(sym(), nullptr);
        uniquePointers.insert(sym());
    }

    // All symbols should be unique
    EXPECT_EQ(uniquePointers.size(), numSymbols);
}

TEST_F(SymbolTest, SymbolLookupPerformance) {
    // Pre-create some symbols
    const int numSymbols = 1000;
    for (int i = 0; i < numSymbols; ++i) {
        std::string name = "perf_symbol_" + std::to_string(i);
        getsym(name.c_str());
    }

    // Look them up many times
    const int lookups = 10000;
    for (int i = 0; i < lookups; ++i) {
        std::string name = "perf_symbol_" + std::to_string(i % numSymbols);
        P<String> sym = getsym(name.c_str());
        EXPECT_NE(sym(), nullptr);
    }
}
