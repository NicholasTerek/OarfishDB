#include <gtest/gtest.h>

#include "src/include/storage/iterators/block_iterator.hpp"
#include "src/include/storage/sst/block.hpp"

TEST(BlockIteratorBasicTest, Empty) {
    BlockBuilder builder(64);
    Block block = builder.build();
    auto iter = BlockIterator::create_and_seek_to_first(&block);
    EXPECT_FALSE(iter->is_valid());
}

TEST(BlockIteratorBasicTest, Single) {
    BlockBuilder builder(64);
    builder.add("a", "1");
    Block block = builder.build();
    auto iter = BlockIterator::create_and_seek_to_first(&block);
    ASSERT_TRUE(iter->is_valid());
    EXPECT_EQ(iter->key(), "a");
    EXPECT_EQ(iter->value(), "1");
    iter->next();
    EXPECT_FALSE(iter->is_valid());
}
