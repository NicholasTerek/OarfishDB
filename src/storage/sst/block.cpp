#include "src/include/storage/sst/block.hpp"

#include <cstring>

#include "src/include/common/util/encoding.hpp"

BlockBuilder::BlockBuilder(std::uint16_t target_size)
    : target_size_(target_size) {}

bool BlockBuilder::add(const std::string& key, const std::string& value) {
    if (key.empty()) {
        return false;
    }

    if (key.size() > 65535 || value.size() > 65535) {
        return false;
    }

    std::size_t entry_size = sizeof(std::uint16_t) + key.size() + sizeof(std::uint16_t) + value.size();
    std::size_t next_data_size = data_.size() + entry_size;
    std::size_t next_offsets_size = (offsets_.size() + 1 + 1) * sizeof(std::uint16_t);
    std::size_t next_total_size = next_data_size + next_offsets_size;

    if (!offsets_.empty() && next_total_size > target_size_) {
        return false;
    }

    if (next_data_size > 65535) {
        if (!offsets_.empty()) {
            return false;
        }
    }

    offsets_.push_back(static_cast<std::uint16_t>(data_.size()));
    put_u16(data_, static_cast<std::uint16_t>(key.size()));
    data_.append(key);
    put_u16(data_, static_cast<std::uint16_t>(value.size()));
    data_.append(value);
    return true;
}

Block BlockBuilder::build() {
    Block block(data_, offsets_);
    reset();
    return block;
}

void BlockBuilder::reset() {
    data_.clear();
    offsets_.clear();
}

std::size_t BlockBuilder::data_size() const {
    return data_.size();
}

std::size_t BlockBuilder::num_entries() const {
    return offsets_.size();
}

Block::Block() = default;

Block::Block(std::string data, std::vector<std::uint16_t> offsets)
    : data_(std::move(data)), offsets_(std::move(offsets)) {}

const std::string& Block::data() const {
    return data_;
}

const std::vector<std::uint16_t>& Block::offsets() const {
    return offsets_;
}

std::size_t Block::num_entries() const {
    return offsets_.size();
}

std::string Block::encode() const {
    std::string encoded = data_;
    encoded.reserve(data_.size() + (offsets_.size() + 1) * sizeof(std::uint16_t));
    for (std::uint16_t offset : offsets_) {
        put_u16(encoded, offset);
    }
    put_u16(encoded, static_cast<std::uint16_t>(offsets_.size()));
    return encoded;
}

Block Block::decode(const std::string& encoded) {
    if (encoded.size() < sizeof(std::uint16_t)) {
        return Block();
    }

    std::uint16_t num_entries = 0;
    if (!get_u16(encoded, encoded.size() - sizeof(std::uint16_t), num_entries)) {
        return Block();
    }

    std::size_t offsets_bytes = static_cast<std::size_t>(num_entries) * sizeof(std::uint16_t);
    if (encoded.size() < offsets_bytes + sizeof(std::uint16_t)) {
        return Block();
    }

    std::size_t offsets_start = encoded.size() - sizeof(std::uint16_t) - offsets_bytes;
    std::string data = encoded.substr(0, offsets_start);

    std::vector<std::uint16_t> offsets;
    offsets.reserve(num_entries);
    for (std::size_t i = 0; i < num_entries; ++i) {
        std::uint16_t offset = 0;
        std::size_t off = offsets_start + i * sizeof(std::uint16_t);
        if (!get_u16(encoded, off, offset)) {
            return Block();
        }
        offsets.push_back(offset);
    }

    return Block(std::move(data), std::move(offsets));
}
