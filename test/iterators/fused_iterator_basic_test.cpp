#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/include/iterators/lsm_iterator.hpp"

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

TEST(FusedIteratorBasicTest, Empty) {
    auto inner = std::make_unique<VecIterator>(
        std::vector<std::pair<std::string, std::string>>{});
    auto fused = FusedIterator::create(std::move(inner));
    EXPECT_FALSE(fused->is_valid());
}

TEST(FusedIteratorBasicTest, Single) {
    auto inner = std::make_unique<VecIterator>(
        std::vector<std::pair<std::string, std::string>>{{"a", "1"}});
    auto fused = FusedIterator::create(std::move(inner));
    ASSERT_TRUE(fused->is_valid());
    EXPECT_EQ(fused->key(), "a");
    EXPECT_EQ(fused->value(), "1");
    fused->next();
    EXPECT_FALSE(fused->is_valid());
}
