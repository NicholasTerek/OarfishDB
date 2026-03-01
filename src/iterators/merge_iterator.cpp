#include "../include/iterators/merge_iterator.hpp"
#include <string>

std::unique_ptr<MergeIterator> MergeIterator::create(std::vector<std::unique_ptr<StorageIterator>> iterators) {
    MergeIterator* merge_iter = new MergeIterator();
    
    if (iterators.empty()) {
        return std::unique_ptr<MergeIterator>(merge_iter);
    }
    
    merge_iter->owned_iters_ = std::move(iterators);
    
    for (size_t i = 0; i < merge_iter->owned_iters_.size(); i++) {
        StorageIterator* iter = merge_iter->owned_iters_[i].get();
        if (iter->is_valid()) {
            merge_iter->heap_.push(HeapWrapper(i, iter));
        }
    }
    
    if (!merge_iter->heap_.empty()) {
        HeapWrapper top = merge_iter->heap_.top();
        merge_iter->heap_.pop();
        merge_iter->current_ = new HeapWrapper(top.index, top.iterator);
    }
    
    return std::unique_ptr<MergeIterator>(merge_iter);
}

std::string MergeIterator::key() {
    if (!current_ || !current_->iterator) {
        return "";
    }
    return current_->iterator->key();
}

std::string MergeIterator::value() {
    if (!current_ || !current_->iterator) {
        return "";
    }
    return current_->iterator->value();
}

bool MergeIterator::is_valid() {
    if (!current_ || !current_->iterator) {
        return false;
    }
    return current_->iterator->is_valid();
}

int MergeIterator::num_active_iterators() {
    int total = 0;
    for (const auto& it : owned_iters_) {
        if (it) {
            total += it->num_active_iterators();
        }
    }
    return total;
}

void MergeIterator::next() {
    if (!current_ || !current_->iterator) {
        return;
    }
    
    std::string current_key = current_->iterator->key();
    
    std::vector<HeapWrapper> to_reinsert;
    while (!heap_.empty()) {
        HeapWrapper top = heap_.top();
        if (top.iterator->key() == current_key) {
            heap_.pop();
            top.iterator->next();
            if (top.iterator->is_valid()) {
                to_reinsert.push_back(top);
            }
        } else {
            break;
        }
    }
    
    current_->iterator->next();
    
    for (size_t i = 0; i < to_reinsert.size(); i++) {
        heap_.push(to_reinsert[i]);
    }

    if (current_->iterator->is_valid()) {
        if (!heap_.empty()) {
            HeapWrapper top = heap_.top();
            if (*current_ < top) {
                heap_.pop();
                heap_.push(*current_);
                delete current_;
                current_ = new HeapWrapper(top.index, top.iterator);
            }
        }
    } else {
        delete current_;
        current_ = nullptr;
        
        if (!heap_.empty()) {
            HeapWrapper top = heap_.top();
            heap_.pop();
            current_ = new HeapWrapper(top.index, top.iterator);
        }
    }
}
