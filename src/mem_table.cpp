#include "include/mem_table.hpp"


MemTable::MemTable() {
    id_ = 0;
    approximatesize_ = 0;
}

MemTable::~MemTable(){

}

int MemTable::Id() {
    return id_;
}

int MemTable::Size() {
    return approximatesize_;
}
bool MemTable::isEmpty(){
    return map_.isEmpty();
}

void MemTable::Clear() {
    return; //todo
}

std::optional<std::string> MemTable::get(std::string key){
    std::optional<std::string> found = map_.Contains(key);
    return found;
}

bool MemTable::put(std::string key, std::string value){
    auto old = map_.Contains(key);
    if (old.has_value()) {
        int delta = value.size() - old->size();
        approximatesize_.fetch_add(delta);
    } else {
        approximatesize_.fetch_add(key.size() + value.size());
    }

    map_.Insert(key, value);
    return true;
}

SsTable MemTable::flush(const std::string& file_path, std::uint32_t block_size) const {
    SsTableBuilder builder(block_size);
    auto iter = begin();
    while (iter.is_valid()) {
        builder.add(iter.key(), iter.value());
        iter.next();
    }
    return builder.build_for_test(file_path);
}

// MemTableIterator constructors
MemTable::MemTableIterator::MemTableIterator() 
    : current_node_(nullptr) {}

MemTable::MemTableIterator::MemTableIterator(SkipList::Node* current)
    : current_node_(current) {}

// MemTableIterator methods
std::string MemTable::MemTableIterator::key() {
    return current_node_ ? current_node_->key : "";
}

std::string MemTable::MemTableIterator::value() {
    return current_node_ ? current_node_->value : "";
}

bool MemTable::MemTableIterator::is_valid() {
    return current_node_ != nullptr;
}

int MemTable::MemTableIterator::num_active_iterators() {
    return 1;
}

void MemTable::MemTableIterator::next() {
    if (current_node_) {
        current_node_ = current_node_->next[0];
    }
}

// MemTable iterator factory methods
MemTable::MemTableIterator MemTable::begin() const {
    return MemTableIterator(map_.head_->next[0]);
}

MemTable::MemTableIterator MemTable::scan(const std::string& lower_bound, const std::string& upper_bound) const {
    auto skip_iter = map_.scan(lower_bound);
    // Since MemTable is a friend of SkipList, we can access the private current_ member
    return MemTableIterator(skip_iter.current_);
}

std::unique_ptr<MemTable::MemTableIterator> MemTable::begin_ptr() const {
    return std::make_unique<MemTableIterator>(map_.head_->next[0]);
}

std::unique_ptr<MemTable::MemTableIterator> MemTable::scan_ptr(const std::string& lower_bound, const std::string& upper_bound) const {
    auto skip_iter = map_.scan(lower_bound);
    return std::make_unique<MemTableIterator>(skip_iter.current_);
}
