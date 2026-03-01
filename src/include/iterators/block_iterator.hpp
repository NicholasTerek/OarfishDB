#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "src/include/iterators/StorageIterator.hpp"

class Block;

class BlockIterator : public StorageIterator {
public:
    explicit BlockIterator(const Block* block);

    static std::unique_ptr<BlockIterator> create_and_seek_to_first(const Block* block);
    static std::unique_ptr<BlockIterator> create_and_seek_to_key(const Block* block, const std::string& key);

    std::string key() override;
    std::string value() override;
    bool is_valid() override;
    void next() override;
    int num_active_iterators() override;

    void seek_to_first();
    void seek_to_key(const std::string& key);

private:
    const Block* block_;
    std::size_t index_;
    std::string current_key_;
    std::size_t value_begin_;
    std::size_t value_end_;
    bool valid_;

    void load_at_index(std::size_t index);
    bool key_at_index(std::size_t index, std::string& key_out) const;
};
