#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class BloomFilter {
public:
    BloomFilter();
    ~BloomFilter();

    void Add(const std::string &key);
    bool MayContain(const std::string &key) const;

private:
    void SetBit(std::size_t pos);
    bool GetBit(std::size_t pos) const;

private:
    std::vector<uint8_t> bits_;
    std::size_t num_bits_;
    std::size_t num_hashes_;
};
