#include "include/lsm_storage.hpp"
#include "include/iterators/lsm_iterator.hpp"
#include "include/iterators/merge_iterator.hpp"
#include "include/iterators/sstable_iterator.hpp"
#include "include/iterators/two_merge_iterator.hpp"
#include <algorithm>
#include <memory>
#include <vector>
#include <filesystem>
#include <chrono>

LsmStorageState::LsmStorageState() {
    // Initialize with memtable of id 0
    memtable = new MemTable();
}

LsmStorageState::~LsmStorageState() {
    delete memtable;
    for (MemTable* mt : imm_memtables) {
        delete mt;
    }
}

LsmStorageState LsmStorageState::create() {
    return LsmStorageState();
}


LsmStorageInner::LsmStorageInner() {
    // state_ is initialized automatically by its constructor
    target_sst_size_ = 2 * 1024 * 1024;
    num_memtable_limit_ = 4;
    block_size_ = 4096;
    next_sst_id_ = 1;
    db_path_ = ".";
}

LsmStorageInner::LsmStorageInner(const std::string& db_path, const LsmStorageOptions& options)
    : target_sst_size_(options.target_sst_size),
      next_sst_id_(1),
      num_memtable_limit_(options.num_memtable_limit),
      block_size_(options.block_size),
      db_path_(db_path) {}

LsmStorageInner::~LsmStorageInner() {
    // Destructor handles cleanup automatically
}

std::shared_ptr<LsmStorageInner> LsmStorageInner::open(const std::string& db_path,
                                                       const LsmStorageOptions& options) {
    std::filesystem::create_directories(db_path);
    return std::make_shared<LsmStorageInner>(db_path, options);
}

std::optional<std::string> LsmStorageInner::get(const std::string& key) {
    LsmStorageSnapshot snapshot = snapshot_state();

    // Search on the current memtable first (newest data)
    std::optional<std::string> result = snapshot.memtable->get(key);

    if (result.has_value()) {
        if (result.value().empty()) {
            return std::nullopt;
        }
        return result.value();
    }

    // Search on immutable memtables from newest to oldest
    for (MemTable* memtable : snapshot.imm_memtables) {
        std::optional<std::string> imm_result = memtable->get(key);
        if (imm_result.has_value()) {
            if (imm_result.value().empty()) {
                return std::nullopt;
            }
            return imm_result.value();
        }
    }

    if (snapshot.l0_sstables.empty()) {
        return std::nullopt;
    }

    std::vector<std::unique_ptr<StorageIterator>> sst_iters;
    sst_iters.reserve(snapshot.l0_sstables.size());
    for (int sst_id : snapshot.l0_sstables) {
        auto it = snapshot.sstables.find(sst_id);
        if (it == snapshot.sstables.end()) {
            continue;
        }
        const std::string first_key = it->second->first_key();
        const std::string last_key = it->second->last_key();
        if (!first_key.empty() && (key < first_key || key > last_key)) {
            continue;
        }
        auto iter = SsTableIterator::create_and_seek_to_key(it->second, key);
        sst_iters.push_back(std::move(iter));
    }

    if (sst_iters.empty()) {
        return std::nullopt;
    }

    auto merge_iter = MergeIterator::create(std::move(sst_iters));
    if (!merge_iter->is_valid()) {
        return std::nullopt;
    }
    if (merge_iter->key() != key) {
        return std::nullopt;
    }
    if (merge_iter->value().empty()) {
        return std::nullopt;
    }
    return merge_iter->value();
}

void LsmStorageInner::put(const std::string& key, const std::string& value) {
    // Put a key-value pair into the storage by writing into the current memtable
    state_.memtable->put(key, value);
    
    // Check if memtable should be frozen after put
    int estimated_size = state_.memtable->Size();
    try_freeze(estimated_size);
}

void LsmStorageInner::delete_key(const std::string& key) {
    // Remove a key from the storage by writing an empty value (tombstone)
    state_.memtable->put(key, "");
    
    // Check if memtable should be frozen after delete (tombstone still takes space)
    int estimated_size = state_.memtable->Size();
    try_freeze(estimated_size);
}

void LsmStorageInner::force_freeze_memtable() {
    std::lock_guard<std::mutex> lock(state_lock_);
    
    // Force freeze regardless of size (as the name suggests)
    // Move current memtable to immutable list and create new one
    MemTable* old_memtable = state_.memtable;
    
    // Add to immutable memtables (latest first)
    state_.imm_memtables.insert(state_.imm_memtables.begin(), old_memtable);
    
    // Create new current memtable
    state_.memtable = new MemTable();
}

bool LsmStorageInner::force_flush_next_imm_memtable() {
    MemTable* mem_to_flush = nullptr;
    int sst_id = 0;
    std::string path;
    {
        std::lock_guard<std::mutex> lock(state_lock_);
        if (state_.imm_memtables.empty()) {
            return false;
        }
        mem_to_flush = state_.imm_memtables.back();
        sst_id = next_sst_id_++;
        path = db_path_ + "/" + std::to_string(sst_id) + ".sst";
    }

    SsTable sst = mem_to_flush->flush(path, static_cast<std::uint32_t>(block_size_));

    {
        std::lock_guard<std::mutex> lock(state_lock_);
        if (!state_.imm_memtables.empty() && state_.imm_memtables.back() == mem_to_flush) {
            state_.imm_memtables.pop_back();
        } else {
            auto it = std::find(state_.imm_memtables.begin(), state_.imm_memtables.end(), mem_to_flush);
            if (it != state_.imm_memtables.end()) {
                state_.imm_memtables.erase(it);
            }
        }
        state_.l0_sstables.insert(state_.l0_sstables.begin(), sst_id);
        state_.sstables[sst_id] = std::make_shared<SsTable>(std::move(sst));
    }

    delete mem_to_flush;
    return true;
}

void LsmStorageInner::trigger_flush() {
    while (true) {
        int total = 0;
        int imm = 0;
        {
            std::lock_guard<std::mutex> lock(state_lock_);
            imm = static_cast<int>(state_.imm_memtables.size());
            total = imm + 1;
        }
        if (total <= num_memtable_limit_ || imm == 0) {
            break;
        }
        force_flush_next_imm_memtable();
    }
}

void LsmStorageInner::add_l0_sstable(int sst_id, std::shared_ptr<SsTable> sst) {
    std::lock_guard<std::mutex> lock(state_lock_);
    state_.l0_sstables.push_back(sst_id);
    state_.sstables[sst_id] = std::move(sst);
}

int LsmStorageInner::next_sst_id() {
    return next_sst_id_++;
}

std::unique_ptr<FusedIterator> LsmStorageInner::scan() {
    return scan(Bound::unbounded(), Bound::unbounded());
}

std::unique_ptr<FusedIterator> LsmStorageInner::scan(const Bound& lower, const Bound& upper) {
    LsmStorageSnapshot snapshot = snapshot_state();

    std::vector<std::unique_ptr<StorageIterator>> mem_iters;
    mem_iters.push_back(snapshot.memtable->scan_ptr(lower.key, upper.key));
    for (size_t i = 0; i < snapshot.imm_memtables.size(); i++) {
        mem_iters.push_back(snapshot.imm_memtables[i]->scan_ptr(lower.key, upper.key));
    }
    auto mem_merge = MergeIterator::create(std::move(mem_iters));

    std::vector<std::unique_ptr<StorageIterator>> sst_iters;
    for (int sst_id : snapshot.l0_sstables) {
        auto it = snapshot.sstables.find(sst_id);
        if (it == snapshot.sstables.end()) {
            continue;
        }
        const std::string first_key = it->second->first_key();
        const std::string last_key = it->second->last_key();
        bool before_lower = false;
        bool after_upper = false;
        if (lower.type == BoundType::Included) {
            before_lower = last_key < lower.key;
        } else if (lower.type == BoundType::Excluded) {
            before_lower = last_key <= lower.key;
        }
        if (upper.type == BoundType::Included) {
            after_upper = first_key > upper.key;
        } else if (upper.type == BoundType::Excluded) {
            after_upper = first_key >= upper.key;
        }
        if (before_lower || after_upper) {
            continue;
        }
        auto iter = SsTableIterator::create_and_seek_to_key(it->second, lower.key);
        sst_iters.push_back(std::move(iter));
    }
    auto sst_merge = MergeIterator::create(std::move(sst_iters));

    auto two_merge = TwoMergeIterator::create(std::move(mem_merge), std::move(sst_merge));
    auto lsm_iter = LsmIterator::create_with_end(std::move(two_merge), upper);

    if (lower.type == BoundType::Excluded) {
        while (lsm_iter->is_valid() && lsm_iter->key() == lower.key) {
            lsm_iter->next();
        }
    }

    return FusedIterator::create(std::move(lsm_iter));
}

// Wrapper implementation
std::unique_ptr<FusedIterator> Lsm::scan() {
    return inner_->scan();
}

std::unique_ptr<FusedIterator> Lsm::scan(const Bound& lower, const Bound& upper) {
    return inner_->scan(lower, upper);
}

bool LsmStorageInner::try_freeze(int estimated_size) {
    if (estimated_size >= target_sst_size_) {
        std::lock_guard<std::mutex> lock(state_lock_);
        
        if (state_.memtable->Size() >= target_sst_size_) {
            // Inline the freeze logic to avoid deadlock
            MemTable* old_memtable = state_.memtable;
            state_.imm_memtables.insert(state_.imm_memtables.begin(), old_memtable);
            
            // Create new current memtable
            state_.memtable = new MemTable();
            return true;
        }
    }
    return false;
}

LsmStorageSnapshot LsmStorageInner::snapshot_state() {
    std::lock_guard<std::mutex> lock(state_lock_);
    LsmStorageSnapshot snapshot;
    snapshot.memtable = state_.memtable;
    snapshot.imm_memtables = state_.imm_memtables;
    snapshot.l0_sstables = state_.l0_sstables;
    snapshot.sstables = state_.sstables;
    return snapshot;
}

LsmStorageOptions LsmStorageOptions::default_for_week1_test() {
    LsmStorageOptions options;
    options.block_size = 128;
    options.target_sst_size = 2 * 1024 * 1024;
    options.num_memtable_limit = 4;
    options.flush_interval_ms = 50;
    return options;
}

LsmStorageOptions LsmStorageOptions::default_for_week1_day6_test() {
    LsmStorageOptions options;
    options.block_size = 128;
    options.target_sst_size = 2 * 1024 * 1024;
    options.num_memtable_limit = 2;
    options.flush_interval_ms = 50;
    return options;
}

// Test accessors
int LsmStorageInner::get_imm_memtables_count() const {
    return state_.imm_memtables.size();
}

int LsmStorageInner::get_l0_sstables_count() const {
    return state_.l0_sstables.size();
}

int LsmStorageInner::get_imm_memtable_size(int index) const {
    if (index >= 0 && index < static_cast<int>(state_.imm_memtables.size())) {
        return state_.imm_memtables[index]->Size();
    }
    return 0;
}

void LsmStorageInner::set_target_sst_size(int size) {
    target_sst_size_ = size;
}

int LsmStorageInner::get_current_memtable_size() const {
    return state_.memtable->Size();
}


Lsm::Lsm() {
    inner_ = std::make_shared<LsmStorageInner>();
    stop_flush_ = true;
    flush_interval_ms_ = 50;
}

Lsm::~Lsm() {
    close();
}

std::unique_ptr<Lsm> Lsm::open(const std::string& db_path, const LsmStorageOptions& options) {
    auto lsm = std::unique_ptr<Lsm>(new Lsm());
    lsm->inner_ = LsmStorageInner::open(db_path, options);
    lsm->flush_interval_ms_ = options.flush_interval_ms;
    lsm->stop_flush_ = false;
    lsm->start_flush_thread();
    return lsm;
}

void Lsm::close() {
    if (!inner_) {
        return;
    }
    if (!stop_flush_) {
        stop_flush_ = true;
        if (flush_thread_.joinable()) {
            flush_thread_.join();
        }
    }
}

std::optional<std::string> Lsm::get(const std::string& key) {
    return inner_->get(key);
}

void Lsm::put(const std::string& key, const std::string& value) {
    inner_->put(key, value);
}

void Lsm::delete_key(const std::string& key) {
    inner_->delete_key(key);
}

int Lsm::get_l0_sstables_count() const {
    return inner_ ? inner_->get_l0_sstables_count() : 0;
}

void Lsm::start_flush_thread() {
    flush_thread_ = std::thread([this]() {
        while (!stop_flush_) {
            inner_->trigger_flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(flush_interval_ms_));
        }
    });
}
