#include "../../src/include/iterators/lsm_iterator.hpp"
#include "../../src/include/iterators/merge_iterator.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <string>

// Simple mock iterator for testing
class MockIterator : public StorageIterator {
private:
    std::vector<std::pair<std::string, std::string>> data_;
    size_t current_index_;

public:
    MockIterator(std::vector<std::pair<std::string, std::string>> data) 
        : data_(data), current_index_(0) {}

    std::string key() override {
        return current_index_ < data_.size() ? data_[current_index_].first : "";
    }

    std::string value() override {
        return current_index_ < data_.size() ? data_[current_index_].second : "";
    }

    bool is_valid() override {
        return current_index_ < data_.size();
    }

    int num_active_iterators() override {
        return 1;
    }

    void next() override {
        if (current_index_ < data_.size()) {
            current_index_++;
        }
    }
};

/**
 * Test 1: LsmIterator filters out deleted keys (empty values)
 */
TEST(LsmIteratorTest, SkipDeletedKeys) {
    // Create data with some deleted keys (empty values are tombstones)
    std::vector<std::pair<std::string, std::string>> data1 = {
        {"a", "1"},
        {"b", ""},   // Deleted key
        {"c", "3"}
    };
    std::vector<std::pair<std::string, std::string>> data2 = {
        {"d", ""},   // Deleted key
        {"e", "5"}
    };
    
    std::vector<std::unique_ptr<StorageIterator>> iters;
    iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data1)));
    iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data2)));
    
    auto merge_iter = MergeIterator::create(std::move(iters));
    auto lsm_iter = LsmIterator::create(std::move(merge_iter));
    
    // Should only see non-deleted keys: a, c, e (b and d are filtered out)
    EXPECT_TRUE(lsm_iter->is_valid());
    EXPECT_EQ(lsm_iter->key(), "a");
    EXPECT_EQ(lsm_iter->value(), "1");
    
    lsm_iter->next();
    EXPECT_TRUE(lsm_iter->is_valid());
    EXPECT_EQ(lsm_iter->key(), "c");
    EXPECT_EQ(lsm_iter->value(), "3");
    
    lsm_iter->next();
    EXPECT_TRUE(lsm_iter->is_valid());
    EXPECT_EQ(lsm_iter->key(), "e");
    EXPECT_EQ(lsm_iter->value(), "5");
    
    lsm_iter->next();
    EXPECT_FALSE(lsm_iter->is_valid());
}

/**
 * Test 2: LsmIterator with all deleted keys
 */
TEST(LsmIteratorTest, AllKeysDeleted) {
    std::vector<std::pair<std::string, std::string>> data = {
        {"a", ""},
        {"b", ""},
        {"c", ""}
    };
    
    std::vector<std::unique_ptr<StorageIterator>> iters;
    iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data)));
    
    auto merge_iter = MergeIterator::create(std::move(iters));
    auto lsm_iter = LsmIterator::create(std::move(merge_iter));
    
    // All keys deleted, iterator should be invalid from the start
    EXPECT_FALSE(lsm_iter->is_valid());
}

/**
 * Test 3: LsmIterator with no deleted keys
 */
TEST(LsmIteratorTest, NoDeletedKeys) {
    std::vector<std::pair<std::string, std::string>> data = {
        {"a", "1"},
        {"b", "2"},
        {"c", "3"}
    };
    
    std::vector<std::unique_ptr<StorageIterator>> iters;
    iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data)));
    
    auto merge_iter = MergeIterator::create(std::move(iters));
    auto lsm_iter = LsmIterator::create(std::move(merge_iter));
    
    // Should see all keys
    EXPECT_TRUE(lsm_iter->is_valid());
    EXPECT_EQ(lsm_iter->key(), "a");
    
    lsm_iter->next();
    EXPECT_EQ(lsm_iter->key(), "b");
    
    lsm_iter->next();
    EXPECT_EQ(lsm_iter->key(), "c");
    
    lsm_iter->next();
    EXPECT_FALSE(lsm_iter->is_valid());
}

/**
 * Test 4: FusedIterator prevents calling methods when invalid
 */
TEST(FusedIteratorTest, PreventInvalidAccess) {
    std::vector<std::pair<std::string, std::string>> data = {
        {"a", "1"},
        {"b", "2"}
    };
    
    auto mock_iter = std::unique_ptr<StorageIterator>(new MockIterator(data));
    auto fused_iter = FusedIterator::create(std::move(mock_iter));
    
    // Normal usage
    EXPECT_TRUE(fused_iter->is_valid());
    EXPECT_EQ(fused_iter->key(), "a");
    EXPECT_EQ(fused_iter->value(), "1");
    
    fused_iter->next();
    EXPECT_TRUE(fused_iter->is_valid());
    EXPECT_EQ(fused_iter->key(), "b");
    
    fused_iter->next();
    EXPECT_FALSE(fused_iter->is_valid());
    
    // Calling methods when invalid should be safe (return empty)
    EXPECT_EQ(fused_iter->key(), "");
    EXPECT_EQ(fused_iter->value(), "");
    
    // Calling next when invalid should be no-op
    fused_iter->next();
    EXPECT_FALSE(fused_iter->is_valid());
}

/**
 * Test 5: FusedIterator with empty iterator
 */
TEST(FusedIteratorTest, EmptyIterator) {
    std::vector<std::pair<std::string, std::string>> data = {};
    
    auto mock_iter = std::unique_ptr<StorageIterator>(new MockIterator(data));
    auto fused_iter = FusedIterator::create(std::move(mock_iter));
    
    EXPECT_FALSE(fused_iter->is_valid());
    EXPECT_EQ(fused_iter->key(), "");
    EXPECT_EQ(fused_iter->value(), "");
}

/**
 * Test 6: LsmIterator with deleted key at start
 */
TEST(LsmIteratorTest, DeletedKeyAtStart) {
    std::vector<std::pair<std::string, std::string>> data = {
        {"a", ""},   // Deleted
        {"b", "2"},
        {"c", "3"}
    };
    
    std::vector<std::unique_ptr<StorageIterator>> iters;
    iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data)));
    
    auto merge_iter = MergeIterator::create(std::move(iters));
    auto lsm_iter = LsmIterator::create(std::move(merge_iter));
    
    // Should skip deleted "a" and start at "b"
    EXPECT_TRUE(lsm_iter->is_valid());
    EXPECT_EQ(lsm_iter->key(), "b");
    EXPECT_EQ(lsm_iter->value(), "2");
}

/**
 * Test 7: LsmIterator with consecutive deleted keys
 */
TEST(LsmIteratorTest, ConsecutiveDeletedKeys) {
    std::vector<std::pair<std::string, std::string>> data = {
        {"a", "1"},
        {"b", ""},   // Deleted
        {"c", ""},   // Deleted
        {"d", ""},   // Deleted
        {"e", "5"}
    };
    
    std::vector<std::unique_ptr<StorageIterator>> iters;
    iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data)));
    
    auto merge_iter = MergeIterator::create(std::move(iters));
    auto lsm_iter = LsmIterator::create(std::move(merge_iter));
    
    // Should see: a, e (skip b, c, d)
    EXPECT_TRUE(lsm_iter->is_valid());
    EXPECT_EQ(lsm_iter->key(), "a");
    
    lsm_iter->next();
    EXPECT_TRUE(lsm_iter->is_valid());
    EXPECT_EQ(lsm_iter->key(), "e");
    
    lsm_iter->next();
    EXPECT_FALSE(lsm_iter->is_valid());
}
