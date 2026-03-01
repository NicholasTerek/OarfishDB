#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/include/iterators/lsm_iterator.hpp"
#include "src/include/iterators/merge_iterator.hpp"

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

TEST(LsmIteratorBasicTest, Empty) {
    std::vector<std::unique_ptr<StorageIterator>> iters;
    iters.push_back(std::make_unique<VecIterator>(
        std::vector<std::pair<std::string, std::string>>{}));
    auto merge_iter = MergeIterator::create(std::move(iters));
    auto lsm_iter = LsmIterator::create(std::move(merge_iter));
    EXPECT_FALSE(lsm_iter->is_valid());
}

TEST(LsmIteratorBasicTest, SingleSkipsTombstone) {
    std::vector<std::unique_ptr<StorageIterator>> iters;
    iters.push_back(std::make_unique<VecIterator>(
        std::vector<std::pair<std::string, std::string>>{{"a", ""}, {"b", "2"}}));
    auto merge_iter = MergeIterator::create(std::move(iters));
    auto lsm_iter = LsmIterator::create(std::move(merge_iter));
    ASSERT_TRUE(lsm_iter->is_valid());
    EXPECT_EQ(lsm_iter->key(), "b");
    EXPECT_EQ(lsm_iter->value(), "2");
}
