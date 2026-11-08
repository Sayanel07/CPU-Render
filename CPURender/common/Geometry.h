#pragma once
#include <vector>
#include <glm/vec3.hpp>

#include "types.h"
#include "vertex.h"


struct Geometry {
	Geometry() = default;
    Geometry(const Geometry& _other) = default;
    Geometry(Geometry&& _other) = default;
    Geometry& operator=(const Geometry& _other) = default;
    Geometry& operator=(Geometry&& _other) = default;
    
    Geometry(const std::vector<Vertex>& _vertices, const std::vector<uint32>& _index);
    void FindMinMax();
    
    ~Geometry() = default;
    
    std::vector<Vertex> m_vertices;
    std::vector<uint32> m_indices;
	glm::vec3 m_min = glm::vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()); 
	glm::vec3 m_max = glm::vec3(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
};

