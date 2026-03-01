#pragma once
#include "mem_table.hpp"
#include "src/include/iterators/bound.hpp"
#include "src/include/iterators/lsm_iterator.hpp"
#include "src/include/sst/sstable.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

// Represents the state of the storage engine
class LsmStorageState {
public:
    LsmStorageState();
    ~LsmStorageState();
    
    MemTable* memtable;
    
    std::vector<MemTable*> imm_memtables;

    std::vector<int> l0_sstables;
    std::unordered_map<int, std::shared_ptr<SsTable>> sstables;
    
    static LsmStorageState create();
};

struct LsmStorageSnapshot {
    MemTable* memtable;
    std::vector<MemTable*> imm_memtables;
    std::vector<int> l0_sstables;
    std::unordered_map<int, std::shared_ptr<SsTable>> sstables;
};

// The storage interface of the LSM tree
class LsmStorageInner {
public:
    LsmStorageInner();
    ~LsmStorageInner();
    
    std::optional<std::string> get(const std::string& key);
    void put(const std::string& key, const std::string& value);
    void delete_key(const std::string& key);
    
    std::unique_ptr<FusedIterator> scan();
    std::unique_ptr<FusedIterator> scan(const Bound& lower, const Bound& upper);

    // Force freeze the current memtable to an immutable memtable
    void force_freeze_memtable();

    // Test helper to add SSTs into L0
    void add_l0_sstable(int sst_id, std::shared_ptr<SsTable> sst);
    
    // Test accessors
    int get_imm_memtables_count() const;
    int get_imm_memtable_size(int index) const;
    void set_target_sst_size(int size);
    int get_current_memtable_size() const;

    

private:
    LsmStorageState state_;
    
    // State lock for synchronizing state modifications
    std::mutex state_lock_;
    
    // Configuration
    int target_sst_size_;
    int next_sst_id_;
    
    // Helper to get next SST ID
    int next_sst_id();
    
    // Helper to check if memtable should be frozen
    bool try_freeze(int estimated_size);

    LsmStorageSnapshot snapshot_state();
};

// Thin wrapper for LsmStorageInner and the user interface
class Lsm {
public:
    Lsm();
    ~Lsm();
    

    std::unique_ptr<FusedIterator> scan();
    std::unique_ptr<FusedIterator> scan(const Bound& lower, const Bound& upper);
    
    std::optional<std::string> get(const std::string& key);
    void put(const std::string& key, const std::string& value);
    void delete_key(const std::string& key);

private:
    LsmStorageInner* inner_;
};
