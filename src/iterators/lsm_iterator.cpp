#include "src/include/iterators/lsm_iterator.hpp"
#include "src/include/iterators/merge_iterator.hpp"
#include <memory>

LsmIterator::LsmIterator(std::unique_ptr<StorageIterator> inner, Bound end_bound)
    : LsmIteratorInner_(std::move(inner)),
      end_bound_(std::move(end_bound)),
      ended_(false) {
    skip_deleted_keys();
}

std::unique_ptr<LsmIterator> LsmIterator::create(std::unique_ptr<MergeIterator> merge_iter) {
    return std::unique_ptr<LsmIterator>(
        new LsmIterator(std::unique_ptr<StorageIterator>(std::move(merge_iter)), Bound::unbounded()));
}

std::unique_ptr<LsmIterator> LsmIterator::create_with_end(std::unique_ptr<StorageIterator> inner,
                                                          Bound end_bound) {
    return std::unique_ptr<LsmIterator>(new LsmIterator(std::move(inner), std::move(end_bound)));
}

void LsmIterator::skip_deleted_keys() {
    while (LsmIteratorInner_->is_valid()) {
        if (exceeds_end_bound()) {
            ended_ = true;
            return;
        }
        if (!LsmIteratorInner_->value().empty()) {
            return;
        }
        LsmIteratorInner_->next();
    }
}
std::string LsmIterator::key() {
    if (!LsmIteratorInner_->is_valid() || ended_) {
        return "";
    }
    return LsmIteratorInner_->key();
}

std::string LsmIterator::value() {
    if (!LsmIteratorInner_->is_valid() || ended_) {
        return "";
    }
    return LsmIteratorInner_->value();
}

bool LsmIterator::is_valid() {
    if (ended_) {
        return false;
    }
    if (!LsmIteratorInner_->is_valid()) {
        return false;
    }
    if (exceeds_end_bound()) {
        return false;
    }
    return true;
}

void LsmIterator::next() {
    if (!LsmIteratorInner_->is_valid() || ended_) {
        return;
    }
    LsmIteratorInner_->next();

    skip_deleted_keys();
}

bool LsmIterator::exceeds_end_bound() const {
    if (end_bound_.type == BoundType::Unbounded) {
        return false;
    }
    std::string k = LsmIteratorInner_->key();
    if (end_bound_.type == BoundType::Included) {
        return k > end_bound_.key;
    }
    return k >= end_bound_.key;
}





FusedIterator::FusedIterator(std::unique_ptr<StorageIterator> inner)
    : inner_(std::move(inner)), has_errored_(false) {}

std::unique_ptr<FusedIterator> FusedIterator::create(std::unique_ptr<StorageIterator> inner) {
    return std::unique_ptr<FusedIterator>(new FusedIterator(std::move(inner)));
}

std::string FusedIterator::key() {
    if (has_errored_ || !inner_->is_valid()) {
        return "";
    }
    return inner_->key();
}

std::string FusedIterator::value() {
    if (has_errored_ || !inner_->is_valid()) {
        return "";
    }
    return inner_->value();
}

bool FusedIterator::is_valid() {
    if (has_errored_) {
        return false;
    }
    return inner_->is_valid();
}

void FusedIterator::next() {
    if (has_errored_ || !inner_->is_valid()) {
        return;
    }
    

    inner_->next();
}
