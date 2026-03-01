#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

#include "src/include/iterators/sstable_iterator.hpp"
#include "src/include/sst/sstable.hpp"

namespace {

std::filesystem::path make_temp_dir() {
    auto base = std::filesystem::temp_directory_path();
    for (int i = 0; i < 100; ++i) {
        auto name = "lsm_sst_test_" + std::to_string(std::rand());
        auto dir = base / name;
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directory(dir);
            return dir;
        }
    }
    auto fallback = base / "lsm_sst_test_fallback";
    std::filesystem::create_directory(fallback);
    return fallback;
}

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

std::pair<std::filesystem::path, SsTable> generate_sst() {
    SsTableBuilder builder(128);
    for (std::size_t idx = 0; idx < num_of_keys(); ++idx) {
        builder.add(key_of(idx), value_of(idx));
    }
    auto dir = make_temp_dir();
    auto path = dir / "1.sst";
    return {dir, builder.build_for_test(path.string())};
}

} // namespace

TEST(SsTableTest, SstBuildSingleKey) {
    SsTableBuilder builder(16);
    builder.add("233", "233333");
    auto dir = make_temp_dir();
    auto path = dir / "1.sst";
    builder.build_for_test(path.string());
}

TEST(SsTableTest, SstBuildTwoBlocks) {
    SsTableBuilder builder(16);
    builder.add("11", "11");
    builder.add("22", "22");
    builder.add("33", "11");
    builder.add("44", "22");
    builder.add("55", "11");
    builder.add("66", "22");
    EXPECT_GE(builder.meta.size(), 2u);
    auto dir = make_temp_dir();
    auto path = dir / "1.sst";
    builder.build_for_test(path.string());
}

TEST(SsTableTest, SstBuildAll) {
    generate_sst();
}

TEST(SsTableTest, SstDecode) {
    auto pair = generate_sst();
    SsTable sst = pair.second;
    auto meta = sst.block_meta();
    SsTable new_sst = SsTable::open_for_test((pair.first / "1.sst").string());
    EXPECT_EQ(new_sst.block_meta().size(), meta.size());
    EXPECT_EQ(new_sst.first_key(), key_of(0));
    EXPECT_EQ(new_sst.last_key(), key_of(num_of_keys() - 1));
}

TEST(SsTableTest, SstIterator) {
    auto pair = generate_sst();
    auto sst = std::make_shared<SsTable>(pair.second);
    auto iter = SsTableIterator::create_and_seek_to_first(sst);
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

TEST(SsTableTest, SstSeekKey) {
    auto pair = generate_sst();
    auto sst = std::make_shared<SsTable>(pair.second);
    auto iter = SsTableIterator::create_and_seek_to_key(sst, key_of(0));
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
