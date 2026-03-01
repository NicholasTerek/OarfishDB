#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class Block;

class BlockBuilder {
public:
    explicit BlockBuilder(std::uint16_t target_size);

    bool add(const std::string& key, const std::string& value);
    Block build();

    void reset();
    std::size_t data_size() const;
    std::size_t num_entries() const;

private:
    std::uint16_t target_size_;
    std::string data_;
    std::vector<std::uint16_t> offsets_;
};

class Block {
public:
    Block();
    Block(std::string data, std::vector<std::uint16_t> offsets);

    const std::string& data() const;
    const std::vector<std::uint16_t>& offsets() const;
    std::size_t num_entries() const;

    std::string encode() const;
    static Block decode(const std::string& encoded);

private:
    std::string data_;
    std::vector<std::uint16_t> offsets_;
};
