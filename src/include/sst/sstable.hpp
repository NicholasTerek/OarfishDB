#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "src/include/sst/block.hpp"

struct BlockMeta {
    std::string first_key;
    std::string last_key;
    std::uint32_t offset;
    std::uint32_t size;
};

class SsTable {
public:
    SsTable();
    SsTable(std::string file_path, std::vector<BlockMeta> block_meta);

    static SsTable open_for_test(const std::string& file_path);

    const std::vector<BlockMeta>& block_meta() const;
    std::string first_key() const;
    std::string last_key() const;

    Block read_block(std::size_t block_id) const;

private:
    std::string file_path_;
    std::vector<BlockMeta> block_meta_;
};

class SsTableBuilder {
public:
    explicit SsTableBuilder(std::uint32_t block_size);

    void add(const std::string& key, const std::string& value);
    SsTable build_for_test(const std::string& file_path);

    std::size_t estimated_size() const;

    std::vector<BlockMeta> meta;

private:
    void flush_block();

    std::uint32_t block_size_;
    BlockBuilder block_builder_;
    std::string data_;
    std::string first_key_in_block_;
    std::string last_key_in_block_;
};
