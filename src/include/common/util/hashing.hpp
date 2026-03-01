#pragma once

#include <cstddef>
#include <string>

class hashing {
public:
    hashing() = delete;
    ~hashing() = delete;

    /* MurmurHash is a non-cryptographic hash function suitable for general hash-based lookup.
     *
     */
     static std::size_t murmurHash64(const std::string& key, std::size_t seed = 0);
};
