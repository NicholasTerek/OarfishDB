#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "src/include/storage/iterators/StorageIterator.hpp"
#include "src/include/storage/sst/sstable.hpp"
#include "src/include/storage/iterators/block_iterator.hpp"

class SsTableIterator : public StorageIterator {
public:
    explicit SsTableIterator(std::shared_ptr<SsTable> sst);

    static std::unique_ptr<SsTableIterator> create_and_seek_to_first(std::shared_ptr<SsTable> sst);
    static std::unique_ptr<SsTableIterator> create_and_seek_to_key(std::shared_ptr<SsTable> sst,
                                                                   const std::string& key);

    std::string key() override;
    std::string value() override;
    bool is_valid() override;
    void next() override;
    int num_active_iterators() override;

    void seek_to_first();
    void seek_to_key(const std::string& key);

private:
    std::shared_ptr<SsTable> sst_;
    std::size_t block_id_;
    std::unique_ptr<Block> current_block_;
    std::unique_ptr<BlockIterator> block_iter_;
    bool valid_;

    void load_block(std::size_t block_id);
    void ensure_valid_or_advance();
};
