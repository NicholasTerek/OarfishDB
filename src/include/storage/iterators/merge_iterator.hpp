#pragma once

#include "src/include/storage/iterators/StorageIterator.hpp"
#include <vector>
#include <memory>
#include <queue>

/**
 * HeapWrapper wraps an iterator with its index.
 * Lower index = newer data = higher priority when keys are equal.
 */
struct HeapWrapper {
    size_t index;
    StorageIterator* iterator;
    
    HeapWrapper(size_t idx, StorageIterator* iter) 
        : index(idx), iterator(iter) {}
    
    // For priority_queue: we want min-heap by key, with lower index winning ties
    // operator< is reversed because priority_queue is a max-heap by default
    bool operator<(const HeapWrapper& other) const {
        std::string my_key = iterator->key();
        std::string other_key = other.iterator->key();
        
        if (my_key != other_key) {
            return my_key > other_key;
        }
        
        // If keys equal, prefer lower index
        return index > other.index;
    }
};

/**
 * MergeIterator merges multiple iterators using a binary heap.
 */
class MergeIterator : public StorageIterator {
private:
    std::priority_queue<HeapWrapper> heap_;
    HeapWrapper* current_;
    std::vector<std::unique_ptr<StorageIterator>> owned_iters_;

public:
    /**
     * Create a merge iterator from a vector of iterators.
     * Index 0 has the newest data.
     */
    static std::unique_ptr<MergeIterator> create(std::vector<std::unique_ptr<StorageIterator>> iterators);
    
    // StorageIterator interface
    std::string key() override;
    std::string value() override;
    bool is_valid() override;
    void next() override;
    int num_active_iterators() override;
    
    ~MergeIterator() { delete current_; }

private:
    MergeIterator() : current_(nullptr) {}
};
