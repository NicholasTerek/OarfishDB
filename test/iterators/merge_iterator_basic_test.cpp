#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/include/iterators/merge_iterator.hpp"

namespace {

class VecIterator : public StorageIterator {
public:
    explicit VecIterator(std::vector<std::pair<std::string, std::string>> data)
        : data_(std::move(data)), index_(0) {}

    std::string key() override { return is_valid() ? data_[index_].first : ""; }
    std::string value() override { return is_valid() ? data_[index_].second : ""; }
    bool is_valid() override { return index_ < data_.size(); }
    void next() override { if (index_ < data_.size()) { ++index_; } }

private:
    std::vector<std::pair<std::string, std::string>> data_;
    std::size_t index_;
};

} // namespace

TEST(MergeIteratorBasicTest, Empty) {
    std::vector<std::unique_ptr<StorageIterator>> iters;
    iters.push_back(std::make_unique<VecIterator>(
        std::vector<std::pair<std::string, std::string>>{}));
    auto iter = MergeIterator::create(std::move(iters));
    EXPECT_FALSE(iter->is_valid());
}

TEST(MergeIteratorBasicTest, Single) {
    std::vector<std::unique_ptr<StorageIterator>> iters;
    iters.push_back(std::make_unique<VecIterator>(
        std::vector<std::pair<std::string, std::string>>{{"a", "1"}}));
    auto iter = MergeIterator::create(std::move(iters));
    ASSERT_TRUE(iter->is_valid());
    EXPECT_EQ(iter->key(), "a");
    EXPECT_EQ(iter->value(), "1");
    iter->next();
    EXPECT_FALSE(iter->is_valid());
}
