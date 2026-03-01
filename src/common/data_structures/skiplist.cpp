#include "src/include/common/data_structures/skiplist.hpp"
#include <random>
#include <mutex>

/**
 * Helper function for random number generation used in probabilistic height selection
 * Returns 0 or 1 with equal probability for coin-flip style randomness
 * Static variables ensure thread-local random state for performance
 */
double random_() {
    static std::random_device rd;
    static std::mt19937 engine(rd());
    std::uniform_int_distribution<int> dist(0, 1);
    return dist(engine);
}

// Constructor implementation
SkipList::SkipList() {
    max_level_ = 16;
    level_ = 1;
    size_ = 0;
    prob_ = 0.5;
    head_ = new Node(max_level_);
}

// Destructor implementation
SkipList::~SkipList() {
    Clear();
    delete head_;
    head_ = nullptr;
}

// Thread-safe empty check with shared lock (allows concurrent reads)
bool SkipList::isEmpty() const {
    std::shared_lock<std::shared_mutex> lk(mu_);
    return size_ == 0;
}

// Thread-safe size getter with shared lock (allows concurrent reads)
int SkipList::Size() const {
    std::shared_lock<std::shared_mutex> lk(mu_);
    return size_;
}


// Insert/update implementation with skip list algorithm
void SkipList::Insert(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lk(mu_);

    std::vector<Node*> update(static_cast<size_t>(max_level_), nullptr);
    Node* x = FindGE_(key, update);

    if (x && x->key == key) {
        x->value = value;
        return;
    }

    // Create new node with probabilistically determined height
    int node_level = RandomHeight_();
    if (node_level > level_) {
        for (int i = level_; i < node_level; ++i) 
            update[static_cast<size_t>(i)] = head_;
        level_ = node_level;
    }

    Node* n = new Node(key, value, node_level);
    for (int i = 0; i < node_level; ++i) {
        n->next[static_cast<size_t>(i)] = update[static_cast<size_t>(i)]->next[static_cast<size_t>(i)];
        update[static_cast<size_t>(i)]->next[static_cast<size_t>(i)] = n;
    }
    ++size_;
    return;
}

// Delete implementation with proper level cleanup
void SkipList::Erase(const std::string& searchKey) {
    std::unique_lock<std::shared_mutex> lk(mu_);
    std::vector<Node*> update(max_level_, nullptr);
    Node* x = FindGE_(searchKey, update);
    if (!x || x->key != searchKey) return;

    for (int i = 0; i < level_; ++i) {
        if (update[i]->next[i] == x) {
            update[i]->next[i] = x->next[i];
        }
    }
    delete x;
    --size_;

    while (level_ > 1 && head_->next[level_ - 1] == nullptr) {
        --level_;
    }
}

// Search implementation using skip list's logarithmic search algorithm
std::optional<std::string> SkipList::Contains(const std::string& searchKey) const {
    std::shared_lock<std::shared_mutex> lk(mu_);
    Node* x = head_;
    for (int i = level_ - 1; i >= 0; --i) {
        while (x->next[i] && x->next[i]->key < searchKey) {
            x = x->next[i];
        }
    }
    Node* y = x->next[0];
    if (y && y->key == searchKey) return y->value;
    return std::nullopt;
}

SkipList::Node* SkipList::FindGE_(const std::string& target, std::vector<Node*>& update) const {
    Node* x = head_;
    for (int i = level_ -1; i >= 0; --i) {
        while (x->next[i] && x->next[i]->key < target) {
            x = x->next[i];
        }
        update[i] = x;
    }
    return x->next[0];
}

// Clear implementation with thread safety
void SkipList::Clear() {
    std::unique_lock<std::shared_mutex> lk(mu_);
    ClearAll_();
    for (int i = 0; i < max_level_; ++i) {
        head_->next[i] = nullptr;
    }
    
    // Reset to initial state
    size_ = 0;
    level_ = 1;
}

// Helper: delete all data nodes by traversing bottom level
void SkipList::ClearAll_() {
    Node* x = head_->next[0];
    while (x) {
        Node* next = x->next[0];
        delete x;
        x = next;
    }
}

// Generate random height using geometric distribution
// Simulates coin flips: keep going up while getting "heads"
int SkipList::RandomHeight_() const {
    int level = 1;
    while (random_() < prob_ and level < max_level_) {
        level++;
    }
    return level;
}

// SkipList Iterator implementations
std::string SkipList::SkipListIterator::key(){
    return current_->key;
}

std::string SkipList::SkipListIterator::value(){
    return current_->value;
}

bool SkipList::SkipListIterator::is_valid(){
    return current_ != nullptr;
}

int SkipList::SkipListIterator::num_active_iterators() {
    return 1;
}

void SkipList::SkipListIterator::next(){
    current_= current_->next[0];
}

// Create iterator starting from first data node
SkipList::SkipListIterator SkipList::begin() const {
    return SkipListIterator(const_cast<SkipList*>(this), head_->next[0]);
}

// Create iterator starting from first node >= start_key (range scan)
SkipList::SkipListIterator SkipList::scan(const std::string& start_key) const {
    // Find first node >= start_key
    std::vector<Node*> update(max_level_);
    Node* start_node = FindGE_(start_key, update);
    
    return SkipListIterator(const_cast<SkipList*>(this), start_node);
}
