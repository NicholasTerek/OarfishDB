#include "src/include/storage/iterators/StorageIterator.hpp"
#include "src/include/common/data_structures/skiplist.hpp"
#include "src/include/storage/sst/sstable.hpp"
#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <memory>

class MemTable {
public:
    MemTable();
    ~MemTable();

    int Id();
    int Size();
    bool isEmpty();
    void Clear();

    std::optional<std::string> get(std::string key);
    bool put(std::string key, std::string value);
    SsTable flush(const std::string& file_path, std::uint32_t block_size) const;

    class MemTableIterator : public StorageIterator {
    public:
        MemTableIterator();
        MemTableIterator(SkipList::Node* current);

        std::string key() override;
        std::string value() override;
        bool is_valid() override;
        void next() override;
        int num_active_iterators() override;

    private:
        SkipList::Node* current_node_;
    };

    MemTableIterator begin() const;
    MemTableIterator scan(const std::string& lower_bound, const std::string& upper_bound) const;
    
    std::unique_ptr<MemTableIterator> begin_ptr() const;
    std::unique_ptr<MemTableIterator> scan_ptr(const std::string& lower_bound, const std::string& upper_bound) const;
private:
    SkipList map_;
    int id_;
    std::atomic<int> approximatesize_;
//  WriteAheadLog log;

};
