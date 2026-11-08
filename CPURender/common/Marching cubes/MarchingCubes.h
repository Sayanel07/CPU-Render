#pragma once
#include <glm/gtc/noise.hpp>

#include "../geometry.h"

// Code inspired by : https://gameidea.org/2023/12/12/marching-cubes-algorithm/
struct VoxelGrid;

struct TerrainParams {
    float frequency = 0.02f;  // noise frequency per voxel
    int hillOctaves = 5;
    int detailOctaves = 3;
    float lacunarity = 2.0f;
    float persistence = 0.5f;
    float groundLevel = 0.45f;  // fraction of height where the ground sits
    float heightScale = 0.5f;   // fraction of height used as gradient range
    float hillAmp = 0.8f;
    float detailAmp = 0.5f;  // 0 = pure heightmap, higher = overhangs/caves
    unsigned seed = 1337;
};

struct MarchingCubes {
    void Generate(int _width, int _height, int _length, float _isoLevel = 0.0f, const TerrainParams& terrain = TerrainParams{});
    
    const Geometry* GetGeometry() const;
private:
    Geometry m_geometry;
    int m_width = 0;
    int m_height = 0;
    int m_length = 0;
    
    void MarchCube(int x, int y, int z, VoxelGrid& grid, std::vector<glm::vec3>& vertices, std::vector<uint32>& indices, std::vector<int32>& edgeCache, float _isoLevel) const;
    
    // Low value = solid, high value = air
    static float Density(const glm::vec3& pos, int height, const TerrainParams& t, const glm::vec3& seedOffset);
    
    static glm::vec3 CalculateInterpolation(glm::ivec3 a,glm::ivec3 b, VoxelGrid& grid, float _isoLevel);
    static std::array<int, 16> GetTriangulation(int x, int y, int z, VoxelGrid& grid, float _isoLevel);
    
    static glm::ivec3 POINTS[8];
    static glm::ivec2 EDGES[12];
    static std::array<int, 16> TRI_TABLE[256];
};

template<typename T>
inline float FractalBrownianMotion(T p, int octaves, float lacunarity, float persistence) {
    float sum = 0.f, amp = 1.f, norm = 0.f;
    for (int i = 0; i < octaves; i++) {
        sum += amp * glm::perlin(p); // Using glm perlin for 3D noise
        norm += amp;
        p *= lacunarity;
        amp *= persistence;
    }
    return sum / norm;   // roughly in [-1, 1]
}