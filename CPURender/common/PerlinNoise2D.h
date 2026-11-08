#pragma once
#include "types.h"
#include <array>

struct PerlinNoise2D {
    PerlinNoise2D();
    PerlinNoise2D(uint32 _seed);
    
    void SetSeed(uint32 _seed);
    uint32 GetSeed() { return m_seed; }
    float Noise(float _x, float _y) const;
    float FractalBrownianMotion(float _x, float _y, uint32 _numOctaves, float frequency = 0.005f, float _persistence = 0.5f, float _lacunarity = 2.f) const;
private:
    std::array<uint8, 256> m_permutationTable;

    uint32 m_seed;
};
