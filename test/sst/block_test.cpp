#include <gtest/gtest.h>

#include <cstdio>
#include <memory>
#include <string>

#include "src/include/sst/block.hpp"
#include "src/include/iterators/block_iterator.hpp"

namespace {

std::string key_of(std::size_t idx) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "key_%03zu", idx * 5);
    return std::string(buf);
}

std::string value_of(std::size_t idx) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "value_%010zu", idx);
    return std::string(buf);
}

std::size_t num_of_keys() {
    return 100;
}

Block generate_block() {
    BlockBuilder builder(10000);
    for (std::size_t idx = 0; idx < num_of_keys(); ++idx) {
        if (!builder.add(key_of(idx), value_of(idx))) {
            ADD_FAILURE() << "Failed to add key " << idx;
            break;
        }
    }
    return builder.build();
}

} // namespace

TEST(BlockTest, BlockBuildSingleKey) {
    BlockBuilder builder(16);
    EXPECT_TRUE(builder.add("233", "233333"));
    Block block = builder.build();
    EXPECT_EQ(block.num_entries(), 1u);
}

TEST(BlockTest, BlockBuildFull) {
    BlockBuilder builder(16);
    EXPECT_TRUE(builder.add("11", "11"));
    EXPECT_FALSE(builder.add("22", "22"));
    Block block = builder.build();
    EXPECT_EQ(block.num_entries(), 1u);
}

TEST(BlockTest, BlockBuildLarge1) {
    BlockBuilder builder(16);
    EXPECT_TRUE(builder.add("11", std::string(100, '1')));
    Block block = builder.build();
    EXPECT_EQ(block.num_entries(), 1u);
}

TEST(BlockTest, BlockBuildLarge2) {
    BlockBuilder builder(16);
    EXPECT_TRUE(builder.add("11", "1"));
    EXPECT_FALSE(builder.add("11", std::string(100, '1')));
}

TEST(BlockTest, BlockBuildAll) {
    Block block = generate_block();
    EXPECT_EQ(block.num_entries(), num_of_keys());
}

TEST(BlockTest, BlockEncode) {
    Block block = generate_block();
    std::string encoded = block.encode();
    EXPECT_FALSE(encoded.empty());
}

TEST(BlockTest, BlockDecode) {
    Block block = generate_block();
    std::string encoded = block.encode();
    Block decoded = Block::decode(encoded);
    EXPECT_EQ(block.offsets(), decoded.offsets());
    EXPECT_EQ(block.data(), decoded.data());
}

TEST(BlockTest, BlockIterator) {
    auto block = std::make_shared<Block>(generate_block());
    auto iter = BlockIterator::create_and_seek_to_first(block.get());
    for (int round = 0; round < 5; ++round) {
        for (std::size_t i = 0; i < num_of_keys(); ++i) {
            ASSERT_TRUE(iter->is_valid());
            EXPECT_EQ(iter->key(), key_of(i));
            EXPECT_EQ(iter->value(), value_of(i));
            iter->next();
        }
        iter->seek_to_first();
    }
}

TEST(BlockTest, BlockSeekKey) {
    auto block = std::make_shared<Block>(generate_block());
    auto iter = BlockIterator::create_and_seek_to_key(block.get(), key_of(0));
    for (std::size_t offset = 1; offset <= 5; ++offset) {
        for (std::size_t i = 0; i < num_of_keys(); ++i) {
            ASSERT_TRUE(iter->is_valid());
            EXPECT_EQ(iter->key(), key_of(i));
            EXPECT_EQ(iter->value(), value_of(i));
            char buf[32];
            std::snprintf(buf, sizeof(buf), "key_%03zu", i * 5 + offset);
            iter->seek_to_key(std::string(buf));
        }
        iter->seek_to_key("k");
    }
}
