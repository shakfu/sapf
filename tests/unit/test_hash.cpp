//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include <gtest/gtest.h>
#include "Hash.hpp"
#include <string>
#include <set>
#include <vector>

// Test fixture for hash function tests
class HashTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Nothing to set up
    }

    void TearDown() override {
        // Nothing to tear down
    }
};

//==============================================================================
// Basic hash function tests
//==============================================================================

TEST_F(HashTest, EmptyStringHash) {
    int32_t hash = Hash("");
    // Just verify it doesn't crash and returns something
    (void)hash;  // Suppress unused variable warning
}

TEST_F(HashTest, SingleCharHash) {
    int32_t hash = Hash("a");
    EXPECT_NE(hash, 0);  // Unlikely to be zero for non-empty string
}

TEST_F(HashTest, ConsistentHash) {
    const char* str = "test_string";
    int32_t hash1 = Hash(str);
    int32_t hash2 = Hash(str);
    EXPECT_EQ(hash1, hash2);
}

TEST_F(HashTest, DifferentStringsDifferentHashes) {
    int32_t hash1 = Hash("hello");
    int32_t hash2 = Hash("world");
    // While collisions are possible, these simple strings should differ
    EXPECT_NE(hash1, hash2);
}

TEST_F(HashTest, SimilarStringsDifferentHashes) {
    int32_t hash1 = Hash("test1");
    int32_t hash2 = Hash("test2");
    EXPECT_NE(hash1, hash2);
}

TEST_F(HashTest, CaseSensitive) {
    int32_t hash1 = Hash("Test");
    int32_t hash2 = Hash("test");
    EXPECT_NE(hash1, hash2);
}

//==============================================================================
// Hash64 tests
//==============================================================================

TEST_F(HashTest, Hash64Basic) {
    uint64_t hash = Hash64(12345ULL);
    EXPECT_NE(hash, 12345ULL);  // Should be transformed
}

TEST_F(HashTest, Hash64Consistent) {
    uint64_t input = 0xDEADBEEFCAFEBABEULL;
    uint64_t hash1 = Hash64(input);
    uint64_t hash2 = Hash64(input);
    EXPECT_EQ(hash1, hash2);
}

TEST_F(HashTest, Hash64DifferentInputsDifferentOutputs) {
    uint64_t hash1 = Hash64(1);
    uint64_t hash2 = Hash64(2);
    EXPECT_NE(hash1, hash2);
}

TEST_F(HashTest, Hash64ZeroInput) {
    uint64_t hash = Hash64(0);
    // Should return something (may or may not be zero depending on algorithm)
    (void)hash;
}

TEST_F(HashTest, Hash64MaxInput) {
    uint64_t hash = Hash64(UINT64_MAX);
    // Should not crash
    (void)hash;
}

//==============================================================================
// Distribution tests (statistical)
//==============================================================================

TEST_F(HashTest, HashDistribution) {
    const int numStrings = 1000;
    const int numBuckets = 16;  // Simulate a small hash table
    std::vector<int> bucketCounts(numBuckets, 0);

    for (int i = 0; i < numStrings; ++i) {
        std::string str = "test_string_" + std::to_string(i);
        int32_t hash = Hash(str.c_str());
        int bucket = std::abs(hash) % numBuckets;
        bucketCounts[bucket]++;
    }

    // Check that distribution is reasonably uniform
    // Expected: ~62.5 per bucket, allow for some variance
    int minCount = *std::min_element(bucketCounts.begin(), bucketCounts.end());
    int maxCount = *std::max_element(bucketCounts.begin(), bucketCounts.end());

    // Very loose bounds - just checking it's not completely broken
    EXPECT_GT(minCount, 10);  // Each bucket should have something
    EXPECT_LT(maxCount, 200); // No bucket should have everything
}

TEST_F(HashTest, UniqueHashesForSequentialStrings) {
    const int numStrings = 100;
    std::set<int32_t> hashes;

    for (int i = 0; i < numStrings; ++i) {
        std::string str = "seq_" + std::to_string(i);
        int32_t hash = Hash(str.c_str());
        hashes.insert(hash);
    }

    // All hashes should be unique for these distinct strings
    // (collisions are possible but unlikely for small sets)
    EXPECT_EQ(hashes.size(), numStrings);
}

//==============================================================================
// Edge cases
//==============================================================================

TEST_F(HashTest, LongString) {
    std::string longStr(10000, 'x');
    int32_t hash = Hash(longStr.c_str());
    // Should not crash and should return something
    (void)hash;
}

TEST_F(HashTest, BinaryData) {
    // String with embedded nulls won't work with C strings
    // but we can test strings with various byte values
    char data[] = {1, 2, 3, 127, -1, 0};  // Ends with null
    int32_t hash = Hash(data);
    (void)hash;
}

TEST_F(HashTest, UnicodeString) {
    // UTF-8 encoded string
    const char* utf8 = "\xC3\xA9\xC3\xA0\xC3\xB9";  // some accented chars
    int32_t hash = Hash(utf8);
    EXPECT_NE(hash, 0);
}

// Note: CombineHashes is not currently implemented in Hash.hpp
// If added in the future, uncomment these tests
