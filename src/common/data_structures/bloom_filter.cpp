#include "src/include/common/data_structures/bloom_filter.hpp"
#include "src/include/common/util/hashing.hpp"

BloomFilter::BloomFilter() {
    num_bits_ = 1024;
    num_hashes_ = 3;
    bits_.resize((num_bits_ + 7) / 8, 0);
}

BloomFilter::~BloomFilter() {
}

void BloomFilter::SetBit(std::size_t pos) {
    std::size_t byte_index = pos / 8;
    std::size_t bit_index = pos % 8;
    bits_[byte_index] |= static_cast<uint8_t>(1U << bit_index);
}

bool BloomFilter::GetBit(std::size_t pos) const {
    std::size_t byte_index = pos / 8;
    std::size_t bit_index = pos % 8;
    bool result = false;

    result = (bits_[byte_index] & static_cast<uint8_t>(1U << bit_index)) != 0;
    return result;
}

void BloomFilter::Add(const std::string &key) {
    std::size_t h1 = hashing::murmurHash64(key, 0);
    std::size_t h2 = hashing::murmurHash64(key, 1);
    std::size_t i = 0;

    if (h2 == 0) {
        h2 = 1;
    }

    while (i < num_hashes_) {
        std::size_t pos = (h1 + i * h2) % num_bits_;
        SetBit(pos);
        i++;
    }
}

bool BloomFilter::MayContain(const std::string &key) const {
    std::size_t h1 = hashing::murmurHash64(key, 0);
    std::size_t h2 = hashing::murmurHash64(key, 1);
    std::size_t i = 0;
    bool result = true;

    if (h2 == 0) {
        h2 = 1;
    }

    while (i < num_hashes_) {
        std::size_t pos = (h1 + i * h2) % num_bits_;
        if (!GetBit(pos)) {
            result = false;
            break;
        }
        i++;
    }

    return result;
}
