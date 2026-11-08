#include "HeightMap.h"

#include <algorithm>

#include "PerlinNoise2D.h"
#include "vertex.h"

void HeightMap::Generate(uint32 width, uint32 length, float minHeight, float maxHeight, uint32 seed, bool useCustomSeed)
{
    PerlinNoise2D noise;
    if (useCustomSeed)
        noise.SetSeed(seed);

    std::vector<Vertex> vertices(static_cast<uint64>(width) * length);
    
    int w = static_cast<int>(width);
    int l = static_cast<int>(length);
    
    #pragma omp parallel for
    for (int i = 0; i < static_cast<int>(vertices.size()); i++) {
        int x = i % w;
        int z = i / w;
        float yMod = noise.FractalBrownianMotion(static_cast<float>(x), static_cast<float>(z), 8, 0.08f);
        yMod += 1;
        yMod *= 0.5f;
        float y = (maxHeight - minHeight) * yMod + minHeight;
        Vertex v;
        v.pos = {x,y,z};
        v.color = {0.7f,0.7f,0.7f,0.7f};
        vertices[i] = v;
    }

    // Normal compute
    #pragma omp parallel for
    for (int i = 0; i < static_cast<int>(vertices.size()); i++) {
        Vertex& vertex = vertices[i];
        int x = i % w;
        int y = i / w;
        // check for neighbors
        Vertex* neighbors[4] = {};
        for (int id = 0; id < 4; id++) {
            constexpr glm::vec<2, int> nearby[4] = {
                {1,0},
                {0,1},
                {-1,0},
                {0,-1},
            };
            
            int nx = x + nearby[id].x;
            int ny = y + nearby[id].y;
            if (nx < 0 || nx >= w)
                continue;
            if (ny < 0 || ny >= l)
                continue;
            neighbors[id] = &vertices[ny * w + nx];
        }
        
        glm::vec3 normal(0.0f);
        int count = 0;
        
        // Take 2 vertices by 2 clock-wise and end with the first and last cross product
        for (int n = 0; n < 4; n++) {
            Vertex* a = neighbors[n];
            Vertex* b = neighbors[(n + 1) % 4];
            if (!a || !b)
                continue;
            normal += glm::cross(b->pos - vertex.pos, a->pos - vertex.pos);
            count++;
        }
        if (count == 0) {
            vertex.normal = glm::vec3(0, 1, 0); 
            continue;
        }
        vertex.normal = glm::normalize(normal);
    }
    
    std::vector<uint32> indices;
    indices.resize(static_cast<uint64>(length - 1) * (width - 1) * 6);
    
    #pragma omp parallel for
    for (int i = 0; i < (vertices.size() - width); i++) {
        int x = i % w;
        int y = i / w;
        if (x == w - 1)
            continue;
        int id = (i - y) * 6;
        
        uint32 i0 = y * width + x;
        uint32 i1 = i0 + 1;
        uint32 i2 = i0 + width;
        uint32 i3 = i2 + 1;
        
        indices[id] = i0;
        indices[id + 1] = i3;
        indices[id + 2] = i1;

        indices[id + 3] = i0;
        indices[id + 4] = i2;
        indices[id + 5] = i3;
    }
    
    m_heightmap = Geometry(vertices, indices);
    m_length = length;
    m_width = width;
    m_minHeight = minHeight;
    m_maxHeight = maxHeight;
}

const Geometry* HeightMap::GetGeometry() const {
    return &m_heightmap;
}
