#include <gtest/gtest.h>
#include "src/include/common/util/hashing.hpp"

TEST(MurmurHash64Test, BasicDeterminism) {
    std::string key = "hello world";

    std::size_t h1 = hashing::murmurHash64(key, 0);
    std::size_t h2 = hashing::murmurHash64(key, 0);
    std::size_t h3 = hashing::murmurHash64(key, 123);

    // Same input + same seed = same hash
    EXPECT_EQ(h1, h2);

    // Same input + different seed = different hash
    EXPECT_NE(h1, h3);

    // Different key != different hash
    EXPECT_NE(h1, hashing::murmurHash64("hello world!", 0));
}
