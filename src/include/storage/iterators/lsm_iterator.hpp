#pragma once
#include "src/include/storage/iterators/StorageIterator.hpp"
#include "bound.hpp"
#include "merge_iterator.hpp"
#include <memory>
#include <string>

/**
 * LSMIterator wraps a MergeIterator over memtables and filters out deleted keys.
 */
class LsmIterator : public StorageIterator {
public:
    explicit LsmIterator(std::unique_ptr<StorageIterator> inner, Bound end_bound);
    static std::unique_ptr<LsmIterator> create(std::unique_ptr<MergeIterator> merge_iter);
    static std::unique_ptr<LsmIterator> create_with_end(std::unique_ptr<StorageIterator> inner,
                                                        Bound end_bound);

    std::string key() override;
    std::string value() override;
    bool is_valid() override;
    void next() override;
    int num_active_iterators() override;

private:
    std::unique_ptr<StorageIterator> LsmIteratorInner_;
    Bound end_bound_;
    bool ended_;
    void skip_deleted_keys();
    bool exceeds_end_bound() const;
};

class FusedIterator : StorageIterator {
public:
    explicit FusedIterator(std::unique_ptr<StorageIterator> inner);
    static std::unique_ptr<FusedIterator> create(std::unique_ptr<StorageIterator> inner);
    std::string key() override;
    std::string value() override;
    bool is_valid() override;
    void next() override;
    int num_active_iterators() override;

private:
    bool has_errored_;
    std::unique_ptr<StorageIterator> inner_;

};
