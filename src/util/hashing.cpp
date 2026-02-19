#include "src/include/util/hashing.hpp"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>

static inline std::uint64_t rotl64(std::uint64_t x, int r) {
    return (x << r) | (x >> (64 - r));
}

static inline std::uint64_t fmix64(std::uint64_t k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdu;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53u;
    k ^= k >> 33;
    return k;
}

static inline std::uint64_t getBlock64(const std::uint8_t* p) {
    std::uint64_t v = 0;
    std::memcpy(&v, p, sizeof(v)); // safe on any alignment
    return v;
}

std::size_t hashing::murmurHash64(const std::string& key, std::size_t seed) {
    const std::uint8_t* data = reinterpret_cast<const std::uint8_t*>(key.data());
    const std::size_t len = key.size();

    // MurmurHash3 x64_128 constants
    const std::uint64_t c1 = 0x87c37b91114253d5ull;
    const std::uint64_t c2 = 0x4cf5ad432745937full;

    std::uint64_t h1 = static_cast<std::uint64_t>(seed);
    std::uint64_t h2 = static_cast<std::uint64_t>(seed);

    // body: 16-byte blocks
    const std::size_t nblocks = len / 16;
    std::size_t i = 0;
    for (i = 0; i < nblocks; ++i) {
        const std::uint8_t* block = data + (i * 16);

        std::uint64_t k1 = getBlock64(block + 0);
        std::uint64_t k2 = getBlock64(block + 8);

        // mix k1 into h1
        k1 *= c1;
        k1 = rotl64(k1, 31);
        k1 *= c2;
        h1 ^= k1;

        h1 = rotl64(h1, 27);
        h1 += h2;
        h1 = h1 * 5ull + 0x52dce729ull;

        // mix k2 into h2
        k2 *= c2;
        k2 = rotl64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

        h2 = rotl64(h2, 31);
        h2 += h1;
        h2 = h2 * 5ull + 0x38495ab5ull;
    }

    // tail
    const std::uint8_t* tail = data + (nblocks * 16);
    std::uint64_t k1 = 0;
    std::uint64_t k2 = 0;

    switch (len & 15) {
        case 15: k2 ^= (std::uint64_t)tail[14] << 48;
        case 14: k2 ^= (std::uint64_t)tail[13] << 40;
        case 13: k2 ^= (std::uint64_t)tail[12] << 32;
        case 12: k2 ^= (std::uint64_t)tail[11] << 24;
        case 11: k2 ^= (std::uint64_t)tail[10] << 16;
        case 10: k2 ^= (std::uint64_t)tail[ 9] << 8;
        case  9:
            k2 ^= (std::uint64_t)tail[ 8] << 0;
            k2 *= c2;
            k2 = rotl64(k2, 33);
            k2 *= c1;
            h2 ^= k2;

        case  8: k1 ^= (std::uint64_t)tail[ 7] << 56;
        case  7: k1 ^= (std::uint64_t)tail[ 6] << 48;
        case  6: k1 ^= (std::uint64_t)tail[ 5] << 40;
        case  5: k1 ^= (std::uint64_t)tail[ 4] << 32;
        case  4: k1 ^= (std::uint64_t)tail[ 3] << 24;
        case  3: k1 ^= (std::uint64_t)tail[ 2] << 16;
        case  2: k1 ^= (std::uint64_t)tail[ 1] << 8;
        case  1:
            k1 ^= (std::uint64_t)tail[ 0] << 0;
            k1 *= c1;
            k1 = rotl64(k1, 31);
            k1 *= c2;
            h1 ^= k1;

        default:
            break;
    }

    // finalization (x64_128)
    h1 ^= (std::uint64_t)len;
    h2 ^= (std::uint64_t)len;

    h1 += h2;
    h2 += h1;

    h1 = fmix64(h1);
    h2 = fmix64(h2);

    h1 += h2;
    h2 += h1;

    // Fold 128 -> 64. Common, good diffusion.
    std::uint64_t out64 = h1 ^ h2;

    std::size_t result = (std::size_t)out64;
    return result;
}
