#pragma once

#include <memory>

#include "src/include/iterators/StorageIterator.hpp"

class TwoMergeIterator : public StorageIterator {
public:
    TwoMergeIterator(std::unique_ptr<StorageIterator> first,
                     std::unique_ptr<StorageIterator> second);

    static std::unique_ptr<TwoMergeIterator> create(std::unique_ptr<StorageIterator> first,
                                                    std::unique_ptr<StorageIterator> second);

    std::string key() override;
    std::string value() override;
    bool is_valid() override;
    void next() override;

private:
    std::unique_ptr<StorageIterator> first_;
    std::unique_ptr<StorageIterator> second_;
    bool use_first_;

    void refresh_current();
};
