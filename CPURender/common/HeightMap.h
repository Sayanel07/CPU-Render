#pragma once
#include "geometry.h"
#include "types.h"

class HeightMap
{
public:
    void Generate(uint32 width, uint32 length, float minHeight, float maxHeight, uint32 seed = 0, bool useCustomSeed = false);
    const Geometry* GetGeometry() const;
private:
    Geometry m_heightmap;
    uint32 m_width = 0;
    uint32 m_length = 0;
    float m_minHeight = 0.0f;
    float m_maxHeight = 0.0f;
};
