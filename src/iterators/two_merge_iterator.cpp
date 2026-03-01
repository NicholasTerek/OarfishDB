#include "src/include/iterators/two_merge_iterator.hpp"

#include <string>

TwoMergeIterator::TwoMergeIterator(std::unique_ptr<StorageIterator> first,
                                   std::unique_ptr<StorageIterator> second)
    : first_(std::move(first)),
      second_(std::move(second)),
      use_first_(true) {
    refresh_current();
}

std::unique_ptr<TwoMergeIterator> TwoMergeIterator::create(std::unique_ptr<StorageIterator> first,
                                                           std::unique_ptr<StorageIterator> second) {
    return std::unique_ptr<TwoMergeIterator>(new TwoMergeIterator(std::move(first), std::move(second)));
}

std::string TwoMergeIterator::key() {
    if (!is_valid()) {
        return "";
    }
    return use_first_ ? first_->key() : second_->key();
}

std::string TwoMergeIterator::value() {
    if (!is_valid()) {
        return "";
    }
    return use_first_ ? first_->value() : second_->value();
}

bool TwoMergeIterator::is_valid() {
    return (first_ && first_->is_valid()) || (second_ && second_->is_valid());
}

int TwoMergeIterator::num_active_iterators() {
    int total = 0;
    if (first_) {
        total += first_->num_active_iterators();
    }
    if (second_) {
        total += second_->num_active_iterators();
    }
    return total;
}

void TwoMergeIterator::next() {
    if (!is_valid()) {
        return;
    }

    if (use_first_) {
        std::string current_key = first_->key();
        first_->next();
        if (second_ && second_->is_valid()) {
            while (second_->is_valid() && second_->key() == current_key) {
                second_->next();
            }
        }
    } else {
        std::string current_key = second_->key();
        second_->next();
        if (first_ && first_->is_valid()) {
            while (first_->is_valid() && first_->key() == current_key) {
                first_->next();
            }
        }
    }

    refresh_current();
}

void TwoMergeIterator::refresh_current() {
    bool first_valid = first_ && first_->is_valid();
    bool second_valid = second_ && second_->is_valid();

    if (!first_valid && !second_valid) {
        use_first_ = true;
        return;
    }
    if (first_valid && !second_valid) {
        use_first_ = true;
        return;
    }
    if (!first_valid && second_valid) {
        use_first_ = false;
        return;
    }

    const std::string& k1 = first_->key();
    const std::string& k2 = second_->key();
    if (k1 <= k2) {
        use_first_ = true;
    } else {
        use_first_ = false;
    }
}
