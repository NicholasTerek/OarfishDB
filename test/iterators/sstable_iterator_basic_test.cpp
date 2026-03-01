#include <gtest/gtest.h>

#include <filesystem>
#include <memory>

#include "src/include/iterators/sstable_iterator.hpp"
#include "src/include/sst/sstable.hpp"

namespace {

std::filesystem::path make_temp_dir() {
    auto base = std::filesystem::temp_directory_path();
    auto dir = base / "lsm_sstable_iter_basic";
    std::filesystem::create_directory(dir);
    return dir;
}

} // namespace

TEST(SsTableIteratorBasicTest, Empty) {
    SsTableBuilder builder(64);
    auto dir = make_temp_dir();
    auto path = dir / "empty.sst";
    SsTable sst = builder.build_for_test(path.string());
    auto iter = SsTableIterator::create_and_seek_to_first(std::make_shared<SsTable>(sst));
    EXPECT_FALSE(iter->is_valid());
}

TEST(SsTableIteratorBasicTest, Single) {
    SsTableBuilder builder(64);
    builder.add("a", "1");
    auto dir = make_temp_dir();
    auto path = dir / "single.sst";
    SsTable sst = builder.build_for_test(path.string());
    auto iter = SsTableIterator::create_and_seek_to_first(std::make_shared<SsTable>(sst));
    ASSERT_TRUE(iter->is_valid());
    EXPECT_EQ(iter->key(), "a");
    EXPECT_EQ(iter->value(), "1");
    iter->next();
    EXPECT_FALSE(iter->is_valid());
}
