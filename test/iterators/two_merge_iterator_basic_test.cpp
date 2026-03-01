#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/include/iterators/two_merge_iterator.hpp"

namespace {

class VecIterator : public StorageIterator {
public:
    explicit VecIterator(std::vector<std::pair<std::string, std::string>> data)
        : data_(std::move(data)), index_(0) {}

    std::string key() override { return is_valid() ? data_[index_].first : ""; }
    std::string value() override { return is_valid() ? data_[index_].second : ""; }
    bool is_valid() override { return index_ < data_.size(); }
    int num_active_iterators() override { return 1; }
    void next() override { if (index_ < data_.size()) { ++index_; } }

private:
    std::vector<std::pair<std::string, std::string>> data_;
    std::size_t index_;
};

} // namespace

TEST(TwoMergeIteratorBasicTest, Empty) {
    auto i1 = std::make_unique<VecIterator>(
        std::vector<std::pair<std::string, std::string>>{});
    auto i2 = std::make_unique<VecIterator>(
        std::vector<std::pair<std::string, std::string>>{});
    auto iter = TwoMergeIterator::create(std::move(i1), std::move(i2));
    EXPECT_FALSE(iter->is_valid());
}

TEST(TwoMergeIteratorBasicTest, SinglePrefersFirst) {
    auto i1 = std::make_unique<VecIterator>(
        std::vector<std::pair<std::string, std::string>>{{"a", "1"}});
    auto i2 = std::make_unique<VecIterator>(
        std::vector<std::pair<std::string, std::string>>{{"a", "x"}});
    auto iter = TwoMergeIterator::create(std::move(i1), std::move(i2));
    ASSERT_TRUE(iter->is_valid());
    EXPECT_EQ(iter->key(), "a");
    EXPECT_EQ(iter->value(), "1");
}
