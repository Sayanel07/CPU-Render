#pragma once
#include <vector>
#include <immintrin.h>


inline uint64_t encodeMorton2DBMI(uint32_t x, uint32_t y) {
    uint64_t x_spread = _pdep_u64(x, 0x5555555555555555ULL);
    uint64_t y_spread = _pdep_u64(y, 0xAAAAAAAAAAAAAAAAULL);
    return x_spread | y_spread;
}

inline void decodeMorton2DBMI(uint64_t encode, uint32_t& out_x, uint32_t& out_y) {
    out_x = static_cast<uint32_t>(_pext_u64(encode, 0x5555555555555555ULL));
    out_y = static_cast<uint32_t>(_pext_u64(encode, 0xAAAAAAAAAAAAAAAAULL));
}

inline uint32_t nextPowerOf2(uint32_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

inline uint64_t MortonBufferCount(uint32_t _width, uint32_t _height) {
    uint32_t max_dim = std::max(_width, _height);
    uint32_t virtualSize = nextPowerOf2(max_dim);
    return static_cast<uint64_t>(virtualSize) * virtualSize;
}

template<typename T>
inline std::vector<T> encodeMorton2D(const T* _table, uint32_t _width, uint32_t _height) {
    std::vector<T> table;
    table.resize(MortonBufferCount(_width, _height));
    for (uint32_t y = 0; y < _height; y++)
    {
        for (uint32_t x = 0; x < _width; x++) 
        {
            uint64_t srcIndex = static_cast<uint64_t>(y) * _width + x;
            table[encodeMorton2DBMI(x, y)] = _table[srcIndex];
        }
    }
    return table;
}

template<typename T>
inline std::vector<T> encodeMorton2D( const std::vector<T>& _table, uint32_t _width, uint32_t _height) {
    std::vector<T> table;
    table.resize(MortonBufferCount(_width, _height));
    for (uint32_t y = 0; y < _height; y++)
    {
        for (uint32_t x = 0; x < _width; x++) 
        {
            uint64_t srcIndex = static_cast<uint64_t>(y) * _width + x;
            table[encodeMorton2DBMI(x, y)] = _table[srcIndex];
        }
    }
    return table;
}

template<typename T>
inline std::vector<T> decodeMorton2D(const std::vector<T>& _toDecode, uint32_t _width, uint32_t _height) {
    std::vector<T> newTable(_width * _height);
    for (uint32_t y = 0; y < _height; y++)
        for (uint32_t x = 0; x < _width; x++)
            newTable[static_cast<uint64_t>(y) * _width + x] = _toDecode[encodeMorton2DBMI(x, y)];
    return newTable;
}

template<typename T>
inline T getMorton2D(const std::vector<T>& _table, uint32_t x, uint32_t y) {
    return _table[encodeMorton2DBMI(x, y)];
}
