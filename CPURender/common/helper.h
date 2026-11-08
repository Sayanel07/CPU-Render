#pragma once
#include "types.h"

// Generic version: works for float, glm::vec3/vec4, or any type with + and * float
template<typename Format>
inline Format Average4(const Format& a, const Format& b, const Format& c, const Format& d) {
    return (a + b + c + d) * 0.25f;
}

// Packed RGBA8 version
template<>
inline uint32 Average4<uint32>(const uint32& a, const uint32& b, const uint32& c, const uint32& d) {
    uint32 out = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        uint32 sum = ((a >> shift) & 0xFF) + ((b >> shift) & 0xFF) + ((c >> shift) & 0xFF) + ((d >> shift) & 0xFF);
        out |= ((sum + 2) >> 2) << shift;   // +2 for rounding
    }
    return out;
}