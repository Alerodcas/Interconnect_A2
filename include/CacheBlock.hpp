#ifndef CACHEBLOCK_HPP
#define CACHEBLOCK_HPP

#include <array>
#include <cstdint>

struct CacheBlock {
    uint32_t tag = 0;
    std::array<uint8_t, 16> data = {};
    bool valid = false;
};

#endif // CACHEBLOCK_HPP
