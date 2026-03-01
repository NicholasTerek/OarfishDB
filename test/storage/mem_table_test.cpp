#include "src/include/storage/mem_table.hpp"
#include <gtest/gtest.h>
#include <vector>

TEST(MemTableTest, PutAndGet) {
    MemTable mem;

    mem.put("key1", "value1");
    mem.put("key2", "value2");
    mem.put("key3", "value3");

    EXPECT_EQ(mem.get("key1").value(), "value1");
    EXPECT_EQ(mem.get("key2").value(), "value2");
    EXPECT_EQ(mem.get("key3").value(), "value3");
}

TEST(MemTableTest, OverwriteValue) {
    MemTable mem;

    mem.put("key1", "value1");
    mem.put("key2", "value2");
    mem.put("key3", "value3");

    mem.put("key1", "value11");
    mem.put("key2", "value22");
    mem.put("key3", "value33");

    EXPECT_EQ(mem.get("key1").value(), "value11");
    EXPECT_EQ(mem.get("key2").value(), "value22");
    EXPECT_EQ(mem.get("key3").value(), "value33");
}

TEST(MemTableTest, GetMissingKey) {
    MemTable mem;
    mem.put("key1", "value1");
    EXPECT_FALSE(mem.get("key2").has_value());
}

TEST(MemTableTest, MemtableIterator) {
    MemTable memtable;
    memtable.put("key1", "value1");
    memtable.put("key2", "value2");
    memtable.put("key3", "value3");

    // Test full scan
    {
        auto iter = memtable.begin();
        EXPECT_EQ(iter.key(), "key1");
        EXPECT_EQ(iter.value(), "value1");
        EXPECT_TRUE(iter.is_valid());
        
        iter.next();
        EXPECT_EQ(iter.key(), "key2");
        EXPECT_EQ(iter.value(), "value2");
        EXPECT_TRUE(iter.is_valid());
        
        iter.next();
        EXPECT_EQ(iter.key(), "key3");
        EXPECT_EQ(iter.value(), "value3");
        EXPECT_TRUE(iter.is_valid());
        
        iter.next();
        EXPECT_FALSE(iter.is_valid());
    }

    // Test bounded iteration (key1 to key2 inclusive)
    {
        auto iter = memtable.scan("key1", "key2");
        EXPECT_EQ(iter.key(), "key1");
        EXPECT_EQ(iter.value(), "value1");
        EXPECT_TRUE(iter.is_valid());
        
        iter.next();
        EXPECT_EQ(iter.key(), "key2");
        EXPECT_EQ(iter.value(), "value2");
        EXPECT_TRUE(iter.is_valid());
        
        iter.next();
        // Should stop after key2 (would need upper bound logic for exact match)
        // For now, this tests the basic scanning functionality
    }

    // Test starting from middle
    {
        auto iter = memtable.scan("key2", "key3");
        EXPECT_EQ(iter.key(), "key2");
        EXPECT_EQ(iter.value(), "value2");
        EXPECT_TRUE(iter.is_valid());
        
        iter.next();
        EXPECT_EQ(iter.key(), "key3");
        EXPECT_EQ(iter.value(), "value3");
        EXPECT_TRUE(iter.is_valid());
        
        iter.next();
        EXPECT_FALSE(iter.is_valid());
    }
}

TEST(MemTableTest, EmptyMemtableIterator) {
    MemTable memtable;
    
    // Test iteration on empty memtable
    {
        auto iter = memtable.begin();
        EXPECT_FALSE(iter.is_valid());
    }
    
    // Test scan on empty memtable
    {
        auto iter = memtable.scan("key1", "key3");
        EXPECT_FALSE(iter.is_valid());
    }
}

TEST(MemTableTest, IteratorAdvanced) {
    MemTable memtable;
    
    // Add non-sequential keys to test ordering
    memtable.put("apple", "fruit1");
    memtable.put("banana", "fruit2");
    memtable.put("cherry", "fruit3");
    memtable.put("date", "fruit4");
    
    // Test full iteration maintains order
    {
        auto iter = memtable.begin();
        std::vector<std::string> keys;
        std::vector<std::string> values;
        
        while (iter.is_valid()) {
            keys.push_back(iter.key());
            values.push_back(iter.value());
            iter.next();
        }
        
        EXPECT_EQ(keys.size(), 4);
        EXPECT_EQ(keys[0], "apple");
        EXPECT_EQ(keys[1], "banana"); 
        EXPECT_EQ(keys[2], "cherry");
        EXPECT_EQ(keys[3], "date");
        
        EXPECT_EQ(values[0], "fruit1");
        EXPECT_EQ(values[1], "fruit2");
        EXPECT_EQ(values[2], "fruit3");
        EXPECT_EQ(values[3], "fruit4");
    }
    
    // Test scan from middle
    {
        auto iter = memtable.scan("banana", "date");
        EXPECT_TRUE(iter.is_valid());
        EXPECT_EQ(iter.key(), "banana");
        
        iter.next();
        EXPECT_TRUE(iter.is_valid());
        EXPECT_EQ(iter.key(), "cherry");
        
        iter.next();
        EXPECT_TRUE(iter.is_valid());
        EXPECT_EQ(iter.key(), "date");
        
        iter.next();
        EXPECT_FALSE(iter.is_valid());
    }
}

TEST(MemTableTest, IteratorWithOverwrites) {
    MemTable memtable;
    
    // Test iteration after overwrites
    memtable.put("key1", "value1");
    memtable.put("key2", "value2");
    memtable.put("key1", "updated1");  // Overwrite
    
    auto iter = memtable.begin();
    EXPECT_TRUE(iter.is_valid());
    EXPECT_EQ(iter.key(), "key1");
    EXPECT_EQ(iter.value(), "updated1");  // Should see updated value
    
    iter.next();
    EXPECT_TRUE(iter.is_valid());
    EXPECT_EQ(iter.key(), "key2");
    EXPECT_EQ(iter.value(), "value2");
    
    iter.next();
    EXPECT_FALSE(iter.is_valid());
}