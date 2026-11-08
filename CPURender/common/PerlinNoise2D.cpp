#include "PerlinNoise2D.h"

#include <random>
#include <glm/vec2.hpp>
#include <glm/detail/func_geometric.inl>

// All this code is inspired by : https://rtouti.github.io/graphics/perlin-noise-algorithm

// Optimized version (less multiplications)
float Fade(float t) {
    return ((6*t - 15)*t + 10)*t*t*t;
}
    
glm::vec2 GetConstantVector(uint8 _value) {
    // v is the value from the permutation table
    switch(_value & 3) {
    case 0: return {1.0f, 1.0f}; 
    case 1: return {-1.0f, 1.0f};
    case 2: return {-1.0f, -1.0f};	
    default: return {1.0f, -1.0f};
    }
}

float Lerp(float t, float a1, float a2) {
    return a1 + t*(a2-a1);
}

PerlinNoise2D::PerlinNoise2D() {
    srand(static_cast<unsigned>(time(nullptr))); 
    SetSeed(rand());
}

PerlinNoise2D::PerlinNoise2D(uint32 _seed) {
    SetSeed(_seed);
}

void PerlinNoise2D::SetSeed(uint32 _seed) {
    for(uint16 i = 0; i < 256; i++) {
        m_permutationTable[i] = static_cast<uint8>(i);
    }

    std::mt19937 rng(_seed);
    std::shuffle(m_permutationTable.begin(), m_permutationTable.end(), rng);
    m_seed = _seed;
}

float PerlinNoise2D::Noise(float _x, float _y) const {
    // Suppose x, y and z are the float input
    float iX = std::floor(_x);
    float iY = std::floor(_y);
    
    int X = static_cast<int>(iX) & 255;
    int Y = static_cast<int>(iY) & 255;
    float xf = _x - iX;
    float yf = _y - iY;
    
    glm::vec2 topRight = glm::vec2(xf-1.0f, yf-1.0f);
    glm::vec2 topLeft = glm::vec2(xf, yf-1.0f);
    glm::vec2 bottomRight = glm::vec2(xf-1.0f, yf);
    glm::vec2 bottomLeft = glm::vec2(xf, yf);
    
    // Select a value in the array for each of the 4 corners
    int X1 = (X + 1) & 255;
    uint8 valueTopRight = m_permutationTable[(m_permutationTable[X1]+Y+1) & 255];
    uint8 valueTopLeft = m_permutationTable[(m_permutationTable[X]+Y+1) & 255];
    uint8 valueBottomRight = m_permutationTable[(m_permutationTable[X1]+Y) & 255];
    uint8 valueBottomLeft = m_permutationTable[(m_permutationTable[X]+Y) & 255];
	
    float dotTopRight = glm::dot(topRight, GetConstantVector(valueTopRight));
    float dotTopLeft = glm::dot(topLeft, GetConstantVector(valueTopLeft));
    float dotBottomRight = glm::dot(bottomRight, GetConstantVector(valueBottomRight));
    float dotBottomLeft = glm::dot(bottomLeft, GetConstantVector(valueBottomLeft));
	
    float u = Fade(xf);
    float v = Fade(yf);
    return Lerp(u,
        Lerp(v, dotBottomLeft, dotTopLeft),
        Lerp(v, dotBottomRight, dotTopRight)
    );
}

float PerlinNoise2D::FractalBrownianMotion(float _x, float _y, uint32 _numOctaves, float _frequency, float _persistence, float _lacunarity) const {
    float result = 0.0f;
    float amplitude = 1.0f;
    float totalAmplitude = 0.f;

    for (uint32_t octave = 0; octave < _numOctaves; octave++) {
        float n = amplitude * Noise(_x * _frequency, _y * _frequency);
        result += n;
        totalAmplitude += amplitude;
        amplitude *= _persistence;
        _frequency *= _lacunarity;
    }
    return result / totalAmplitude;
}
