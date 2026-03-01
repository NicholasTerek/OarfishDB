/*

A skip list is a probabilistic data structure with effective searching 

My Implementation is based on: 
William Pugh. 1990. Skip lists: a probabilistic alternative to balanced trees. 
Commun. ACM 33, 6 (June 1990), 668–676. https://doi.org/10.1145/78973.78977

Author: Nicholas Terek
*/

#pragma once
#include "src/include/iterators/StorageIterator.hpp"
#include <string>
#include <vector>
#include <optional>
#include <shared_mutex>

/**
 * Thread-safe probabilistic skip list implementation
 * Provides O(log n) expected time for search, insert, and delete operations
 * Uses reader-writer locks for concurrent access
 */
class SkipList {
    friend class MemTable;
public:
    /**
     * Node structure for the skip list
     * Each node contains a key-value pair and multiple forward pointers (next array)
     * The height of a node determines how many levels it participates in
     */
    struct Node {
        std::string key;
        std::string value;
        std::vector<Node*> next;
        Node(std::string k, std::string v, int h)
          : key(std::move(k)), value(std::move(v)), next(static_cast<size_t>(h), nullptr) {}
        
        // Constructor for header node (no key/value, just height)
        Node(int h) : key(), value(), next(static_cast<size_t>(h), nullptr) {}
    };

    /**
     * Constructor: Initialize an empty skip list
     * Sets up the header node and default parameters
     */
    SkipList();
    
    /**
     * Destructor: Clean up all nodes and free memory
     */
    ~SkipList();
    
    /**
     * Check if the skip list is empty
     * @return true if the list contains no elements
     */
    bool isEmpty() const;
    
    /**
     * Get the number of elements in the skip list
     * @return current number of key-value pairs stored
     */
    int Size() const;
    
    /**
     * Remove all elements from the skip list
     * Resets the list to initial empty state
     */
    void Clear();

    /**
     * Insert or update a key-value pair in the skip list
     * If key exists, updates the value; otherwise creates new node
     * 
     * @param key The string key to insert/update
     * @param value The string value to associate with the key
     */
    void Insert(const std::string& key, const std::string& value);
    
    /**
     * Remove a key-value pair from the skip list
     * If key doesn't exist, operation has no effect
     * 
     * @param key The key to remove from the list
     */
    void Erase(const std::string& key);
    
    /**
     * Search for a key in the skip list and return its value
     * Uses skip list's O(log n) expected search time
     * 
     * @param key The key to search for
     * @return std::optional containing the value if found, std::nullopt if not found
     */
    std::optional<std::string> Contains(const std::string& key) const;

    // placeholder for later:
    // bool GetResult(const std::string& key);

    /**
     * Iterator class for traversing the skip list in sorted order
     * Provides standard iterator interface: key(), value(), is_valid(), next()
     * Inherits from StorageIterator for compatibility with other storage components
     */
    class SkipListIterator : public StorageIterator {
        friend class MemTable;
    public:
        SkipListIterator() : skiplist_(nullptr), current_(nullptr) {}
        SkipListIterator(SkipList* skiplist, Node* current) 
            : skiplist_(skiplist), current_(current) {}

        /**
         * Get the current key
         * @return key string of current node
         */
        std::string key() override;
        
        /**
         * Get the current value
         * @return value string of current node
         */
        std::string value() override;
        
        /**
         * Check if iterator points to a valid node
         * @return true if iterator is valid, false if at end
         */
        bool is_valid() override;
        int num_active_iterators() override;
        
        /**
         * Move iterator to next node in sorted order
         * Advances along level 0 (bottom level with all nodes)
         */
        void next() override;
    
    private:
        SkipList* skiplist_;
        SkipList::Node* current_;
    };

    /**
     * Get iterator pointing to first element in the skip list
     * @return iterator at the beginning of sorted sequence
     */
    SkipListIterator begin() const;
    
    /**
     * Get iterator pointing to first element >= start_key
     * Useful for range queries and scans
     * @param start_key The key to start scanning from
     * @return iterator positioned at first key >= start_key
     */
    SkipListIterator scan(const std::string& start_key) const;

private:
    int max_level_;
    int level_;
    int size_ ;
    double prob_;
    Node* head_;

    mutable std::shared_mutex mu_;

    /**
     * Find the first node with key >= target
     * Core search algorithm that fills update array with predecessors at each level
     * @param target The key to search for
     * @param update Array to store predecessor nodes at each level
     * @return pointer to first node >= target, or nullptr if not found
     */
    Node* FindGE_(const std::string& target, std::vector<Node*>& update) const;
    
    /**
     * Helper function to delete all data nodes (called by Clear and destructor)
     * Traverses level 0 and deletes each node
     */
    void ClearAll_();
    
    /**
     * Generate random height for new nodes using geometric distribution
     * Each level has prob_ chance of promotion (default 0.5)
     * Expected height is 1/(1-prob_) = 2 for prob_=0.5
     * @return random height between 1 and max_level_
     */
    int RandomHeight_() const;
};
