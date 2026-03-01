#include <gtest/gtest.h>

#include "src/include/mem_table.hpp"

TEST(MemTableIteratorBasicTest, Empty) {
    MemTable memtable;
    auto iter = memtable.begin();
    EXPECT_FALSE(iter.is_valid());
}

TEST(MemTableIteratorBasicTest, Single) {
    MemTable memtable;
    memtable.put("a", "1");
    auto iter = memtable.begin();
    ASSERT_TRUE(iter.is_valid());
    EXPECT_EQ(iter.key(), "a");
    EXPECT_EQ(iter.value(), "1");
    iter.next();
    EXPECT_FALSE(iter.is_valid());
}
