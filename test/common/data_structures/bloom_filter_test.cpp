#include "src/include/common/data_structures/bloom_filter.hpp"
#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(BloomFilterTest, EmptyFilter) {
    std::vector<std::string> check {"Hello World", "Apple", "Orange", "Pen", "Hello Word"};
    BloomFilter bf;

    for (const std::string &str : check) {
        EXPECT_FALSE(bf.MayContain(str));
    }
}

TEST(BloomFilterTest, InsertedKeysMayContain) {
    std::vector<std::string> input {"Hello World", "Apple", "Orange", "Pen", "Hello Word"};
    BloomFilter bf;

    for (const std::string &in : input) {
        bf.Add(in);
    }

    for (const std::string &in : input) {
        EXPECT_TRUE(bf.MayContain(in));
    }
}

TEST(BloomFilterTest, DuplicateInsertions) {
    BloomFilter bf;

    bf.Add("Apple");
    bf.Add("Apple");
    bf.Add("Apple");

    EXPECT_TRUE(bf.MayContain("Apple"));
}
