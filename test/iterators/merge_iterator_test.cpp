#include "../../src/include/iterators/merge_iterator.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <string>

/**
 * MockIterator for testing - simulates a simple in-memory iterator
 */
class MockIterator : public StorageIterator {
private:
    std::vector<std::pair<std::string, std::string>> data_;
    size_t current_index_;
    bool has_error_;
    size_t error_at_index_;

public:
    MockIterator(std::vector<std::pair<std::string, std::string>> data) 
        : data_(data), current_index_(0), has_error_(false), error_at_index_(0) {}
    
    MockIterator(std::vector<std::pair<std::string, std::string>> data, size_t error_at)
        : data_(data), current_index_(0), has_error_(true), error_at_index_(error_at) {}

    std::string key() override {
        if (current_index_ < data_.size()) {
            return data_[current_index_].first;
        }
        return "";
    }

    std::string value() override {
        if (current_index_ < data_.size()) {
            return data_[current_index_].second;
        }
        return "";
    }

    bool is_valid() override {
        return current_index_ < data_.size();
    }

    int num_active_iterators() override {
        return 1;
    }

    void next() override {
        if (has_error_ && current_index_ == error_at_index_) {
            // Simulate error by becoming invalid
            current_index_ = data_.size();
            return;
        }
        if (current_index_ < data_.size()) {
            current_index_++;
        }
    }
};

/**
 * Helper function to check iterator results match expected key-value pairs
 */
void check_iter_result_by_key(
    MergeIterator* iter,
    const std::vector<std::pair<std::string, std::string>>& expected) {
    
    std::vector<std::pair<std::string, std::string>> actual;
    
    while (iter->is_valid()) {
        actual.push_back({iter->key(), iter->value()});
        iter->next();
    }
    
    ASSERT_EQ(actual.size(), expected.size()) 
        << "Iterator produced different number of elements";
    
    for (size_t i = 0; i < expected.size(); i++) {
        EXPECT_EQ(actual[i].first, expected[i].first) 
            << "Key mismatch at position " << i;
        EXPECT_EQ(actual[i].second, expected[i].second) 
            << "Value mismatch at position " << i;
    }
}

/**
 * Test merging multiple iterators with overlapping keys
 * Lower index = newer data, should take precedence
 */
TEST(MergeIteratorTest, Task2Merge1) {
    // Create test data
    std::vector<std::pair<std::string, std::string>> data1 = {
        {"a", "1.1"}, {"b", "2.1"}, {"c", "3.1"}, {"e", ""}
    };
    std::vector<std::pair<std::string, std::string>> data2 = {
        {"a", "1.2"}, {"b", "2.2"}, {"c", "3.2"}, {"d", "4.2"}
    };
    std::vector<std::pair<std::string, std::string>> data3 = {
        {"b", "2.3"}, {"c", "3.3"}, {"d", "4.3"}
    };

    // Test 1: i1, i2, i3 order (i1 is newest)
    {
        std::vector<std::unique_ptr<StorageIterator>> iters;
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data1)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data2)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data3)));
        
        auto merge_iter = MergeIterator::create(std::move(iters));
        
        std::vector<std::pair<std::string, std::string>> expected = {
            {"a", "1.1"},  // from i1 (newest)
            {"b", "2.1"},  // from i1 (newest)
            {"c", "3.1"},  // from i1 (newest)
            {"d", "4.2"},  // from i2 (i1 doesn't have "d")
            {"e", ""}      // from i1
        };
        
        check_iter_result_by_key(merge_iter.get(), expected);
    }

    // Test 2: i3, i1, i2 order (i3 is newest)
    {
        std::vector<std::unique_ptr<StorageIterator>> iters;
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data3)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data1)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data2)));
        
        auto merge_iter = MergeIterator::create(std::move(iters));
        
        std::vector<std::pair<std::string, std::string>> expected = {
            {"a", "1.1"},  // from i1 (i3 doesn't have "a")
            {"b", "2.3"},  // from i3 (newest)
            {"c", "3.3"},  // from i3 (newest)
            {"d", "4.3"},  // from i3 (newest)
            {"e", ""}      // from i1
        };
        
        check_iter_result_by_key(merge_iter.get(), expected);
    }
}

/**
 * Test merging non-overlapping iterators
 */
TEST(MergeIteratorTest, Task2Merge2) {
    std::vector<std::pair<std::string, std::string>> data1 = {
        {"a", "1.1"}, {"b", "2.1"}, {"c", "3.1"}
    };
    std::vector<std::pair<std::string, std::string>> data2 = {
        {"d", "1.2"}, {"e", "2.2"}, {"f", "3.2"}, {"g", "4.2"}
    };
    std::vector<std::pair<std::string, std::string>> data3 = {
        {"h", "1.3"}, {"i", "2.3"}, {"j", "3.3"}, {"k", "4.3"}
    };
    std::vector<std::pair<std::string, std::string>> data4 = {};

    std::vector<std::pair<std::string, std::string>> expected = {
        {"a", "1.1"}, {"b", "2.1"}, {"c", "3.1"},
        {"d", "1.2"}, {"e", "2.2"}, {"f", "3.2"}, {"g", "4.2"},
        {"h", "1.3"}, {"i", "2.3"}, {"j", "3.3"}, {"k", "4.3"}
    };

    // Test 1: i1, i2, i3, i4 order
    {
        std::vector<std::unique_ptr<StorageIterator>> iters;
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data1)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data2)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data3)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data4)));
        
        auto merge_iter = MergeIterator::create(std::move(iters));
        check_iter_result_by_key(merge_iter.get(), expected);
    }

    // Test 2: i2, i4, i3, i1 order
    {
        std::vector<std::unique_ptr<StorageIterator>> iters;
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data2)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data4)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data3)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data1)));
        
        auto merge_iter = MergeIterator::create(std::move(iters));
        check_iter_result_by_key(merge_iter.get(), expected);
    }

    // Test 3: i4, i3, i2, i1 order
    {
        std::vector<std::unique_ptr<StorageIterator>> iters;
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data4)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data3)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data2)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data1)));
        
        auto merge_iter = MergeIterator::create(std::move(iters));
        check_iter_result_by_key(merge_iter.get(), expected);
    }
}

/**
 * Test merging with empty iterators
 */
TEST(MergeIteratorTest, Task2MergeEmpty) {
    // Test 1: All empty iterators
    {
        std::vector<std::unique_ptr<StorageIterator>> iters;
        auto merge_iter = MergeIterator::create(std::move(iters));
        
        std::vector<std::pair<std::string, std::string>> expected = {};
        check_iter_result_by_key(merge_iter.get(), expected);
    }

    // Test 2: One valid, one empty
    {
        std::vector<std::pair<std::string, std::string>> data1 = {
            {"a", "1.1"}, {"b", "2.1"}, {"c", "3.1"}
        };
        std::vector<std::pair<std::string, std::string>> data2 = {};
        
        std::vector<std::unique_ptr<StorageIterator>> iters;
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data1)));
        iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data2)));
        
        auto merge_iter = MergeIterator::create(std::move(iters));
        
        std::vector<std::pair<std::string, std::string>> expected = {
            {"a", "1.1"}, {"b", "2.1"}, {"c", "3.1"}
        };
        
        check_iter_result_by_key(merge_iter.get(), expected);
    }
}

/**
 * Test that iterator correctly handles basic iteration
 */
TEST(MergeIteratorTest, BasicIteration) {
    std::vector<std::pair<std::string, std::string>> data1 = {
        {"a", "1"}, {"c", "3"}
    };
    std::vector<std::pair<std::string, std::string>> data2 = {
        {"b", "2"}, {"d", "4"}
    };
    
    std::vector<std::unique_ptr<StorageIterator>> iters;
    iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data1)));
    iters.push_back(std::unique_ptr<StorageIterator>(new MockIterator(data2)));
    
    auto merge_iter = MergeIterator::create(std::move(iters));
    
    // Should merge in sorted order: a, b, c, d
    EXPECT_TRUE(merge_iter->is_valid());
    EXPECT_EQ(merge_iter->key(), "a");
    EXPECT_EQ(merge_iter->value(), "1");
    
    merge_iter->next();
    EXPECT_TRUE(merge_iter->is_valid());
    EXPECT_EQ(merge_iter->key(), "b");
    EXPECT_EQ(merge_iter->value(), "2");
    
    merge_iter->next();
    EXPECT_TRUE(merge_iter->is_valid());
    EXPECT_EQ(merge_iter->key(), "c");
    EXPECT_EQ(merge_iter->value(), "3");
    
    merge_iter->next();
    EXPECT_TRUE(merge_iter->is_valid());
    EXPECT_EQ(merge_iter->key(), "d");
    EXPECT_EQ(merge_iter->value(), "4");
    
    merge_iter->next();
    EXPECT_FALSE(merge_iter->is_valid());
}
