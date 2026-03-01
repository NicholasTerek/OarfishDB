#include "src/include/storage/iterators/sstable_iterator.hpp"

SsTableIterator::SsTableIterator(std::shared_ptr<SsTable> sst)
    : sst_(std::move(sst)),
      block_id_(0),
      valid_(false) {}

std::unique_ptr<SsTableIterator> SsTableIterator::create_and_seek_to_first(std::shared_ptr<SsTable> sst) {
    auto iter = std::make_unique<SsTableIterator>(std::move(sst));
    iter->seek_to_first();
    return iter;
}

std::unique_ptr<SsTableIterator> SsTableIterator::create_and_seek_to_key(std::shared_ptr<SsTable> sst,
                                                                         const std::string& key) {
    auto iter = std::make_unique<SsTableIterator>(std::move(sst));
    iter->seek_to_key(key);
    return iter;
}

std::string SsTableIterator::key() {
    if (!valid_ || !block_iter_) {
        return "";
    }
    return block_iter_->key();
}

std::string SsTableIterator::value() {
    if (!valid_ || !block_iter_) {
        return "";
    }
    return block_iter_->value();
}

bool SsTableIterator::is_valid() {
    return valid_;
}

int SsTableIterator::num_active_iterators() {
    return 1;
}

void SsTableIterator::next() {
    if (!valid_ || !block_iter_) {
        return;
    }
    block_iter_->next();
    ensure_valid_or_advance();
}

void SsTableIterator::seek_to_first() {
    block_id_ = 0;
    load_block(block_id_);
    if (block_iter_) {
        block_iter_->seek_to_first();
    }
    ensure_valid_or_advance();
}

void SsTableIterator::seek_to_key(const std::string& key) {
    if (!sst_ || sst_->block_meta().empty()) {
        valid_ = false;
        return;
    }
    const auto& meta = sst_->block_meta();
    std::size_t left = 0;
    std::size_t right = meta.size();
    while (left < right) {
        std::size_t mid = left + (right - left) / 2;
        if (meta[mid].first_key <= key) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    block_id_ = (left == 0) ? 0 : left - 1;
    load_block(block_id_);
    if (block_iter_) {
        block_iter_->seek_to_key(key);
    }
    ensure_valid_or_advance();
}

void SsTableIterator::load_block(std::size_t block_id) {
    if (!sst_ || block_id >= sst_->block_meta().size()) {
        current_block_.reset();
        block_iter_.reset();
        valid_ = false;
        return;
    }
    current_block_ = std::make_unique<Block>(sst_->read_block(block_id));
    block_iter_ = std::make_unique<BlockIterator>(current_block_.get());
    valid_ = true;
}

void SsTableIterator::ensure_valid_or_advance() {
    if (!block_iter_) {
        valid_ = false;
        return;
    }
    if (block_iter_->is_valid()) {
        valid_ = true;
        return;
    }
    while (block_id_ + 1 < sst_->block_meta().size()) {
        ++block_id_;
        load_block(block_id_);
        if (!block_iter_) {
            valid_ = false;
            return;
        }
        block_iter_->seek_to_first();
        if (block_iter_->is_valid()) {
            valid_ = true;
            return;
        }
    }
    valid_ = false;
}
