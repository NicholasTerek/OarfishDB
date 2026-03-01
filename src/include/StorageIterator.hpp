// Based on RocksDB cursor style iterator
#pragma  once
#include <string>

class StorageIterator { 
public:
    StorageIterator() {}
    // No copying allowed
    StorageIterator(const StorageIterator&) = delete;
    void operator=(const StorageIterator&) = delete;

    virtual ~StorageIterator() {}
    
    virtual std::string key() = 0;
    virtual std::string value()= 0;
    virtual bool is_valid() = 0;
    virtual void next() = 0;
    virtual int num_active_iterators() = 0;
};
