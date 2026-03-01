#include "src/include/iterators/block_iterator.hpp"

#include <cstring>

#include "src/include/sst/block.hpp"
#include "src/include/util/encoding.hpp"

namespace {
bool read_entry(const std::string& data, std::size_t offset,
                std::string& key_out, std::size_t& value_begin, std::size_t& value_end) {
    std::uint16_t key_len = 0;
    if (!get_u16(data, offset, key_len)) {
        return false;
    }
    std::size_t key_start = offset + sizeof(std::uint16_t);
    std::size_t value_len_offset = key_start + key_len;
    if (value_len_offset + sizeof(std::uint16_t) > data.size()) {
        return false;
    }
    std::uint16_t value_len = 0;
    if (!get_u16(data, value_len_offset, value_len)) {
        return false;
    }
    std::size_t value_start = value_len_offset + sizeof(std::uint16_t);
    std::size_t value_end_local = value_start + value_len;
    if (value_end_local > data.size()) {
        return false;
    }

    key_out = data.substr(key_start, key_len);
    value_begin = value_start;
    value_end = value_end_local;
    return true;
}

} // namespace

BlockIterator::BlockIterator(const Block* block)
    : block_(block),
      index_(0),
      value_begin_(0),
      value_end_(0),
      valid_(false) {}

std::unique_ptr<BlockIterator> BlockIterator::create_and_seek_to_first(const Block* block) {
    auto iter = std::make_unique<BlockIterator>(block);
    iter->seek_to_first();
    return iter;
}

std::unique_ptr<BlockIterator> BlockIterator::create_and_seek_to_key(const Block* block, const std::string& key) {
    auto iter = std::make_unique<BlockIterator>(block);
    iter->seek_to_key(key);
    return iter;
}

std::string BlockIterator::key() {
    return current_key_;
}

std::string BlockIterator::value() {
    if (!valid_) {
        return "";
    }
    const std::string& data = block_->data();
    if (value_end_ > data.size()) {
        return "";
    }
    return data.substr(value_begin_, value_end_ - value_begin_);
}

bool BlockIterator::is_valid() {
    return valid_;
}

void BlockIterator::next() {
    if (!valid_) {
        return;
    }
    ++index_;
    load_at_index(index_);
}

void BlockIterator::seek_to_first() {
    index_ = 0;
    load_at_index(index_);
}

void BlockIterator::seek_to_key(const std::string& key) {
    if (!block_ || block_->num_entries() == 0) {
        valid_ = false;
        return;
    }

    std::size_t left = 0;
    std::size_t right = block_->num_entries();
    while (left < right) {
        std::size_t mid = left + (right - left) / 2;
        std::string mid_key;
        if (!key_at_index(mid, mid_key)) {
            valid_ = false;
            return;
        }
        if (mid_key < key) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }

    index_ = left;
    load_at_index(index_);
}

void BlockIterator::load_at_index(std::size_t index) {
    if (!block_ || index >= block_->num_entries()) {
        valid_ = false;
        current_key_.clear();
        value_begin_ = 0;
        value_end_ = 0;
        return;
    }

    const std::vector<std::uint16_t>& offsets = block_->offsets();
    std::size_t offset = offsets[index];
    std::string key;
    std::size_t value_begin = 0;
    std::size_t value_end = 0;
    if (!read_entry(block_->data(), offset, key, value_begin, value_end)) {
        valid_ = false;
        return;
    }

    current_key_ = std::move(key);
    value_begin_ = value_begin;
    value_end_ = value_end;
    valid_ = true;
}

bool BlockIterator::key_at_index(std::size_t index, std::string& key_out) const {
    if (!block_ || index >= block_->num_entries()) {
        return false;
    }
    const std::vector<std::uint16_t>& offsets = block_->offsets();
    std::size_t offset = offsets[index];
    std::size_t value_begin = 0;
    std::size_t value_end = 0;
    return read_entry(block_->data(), offset, key_out, value_begin, value_end);
}
