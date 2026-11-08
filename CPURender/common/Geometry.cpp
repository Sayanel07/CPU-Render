#include "geometry.h"

Geometry::Geometry(const std::vector<Vertex>& _vertices, const std::vector<uint32>& _index) : m_vertices(_vertices), m_indices(_index) 
{
    FindMinMax();
}

void Geometry::FindMinMax()
{
    for (const Vertex& vertex : m_vertices)
    {
        // Get Bounding Box
        m_max = glm::max(m_max, vertex.pos);
        m_min = glm::min(m_min, vertex.pos);
    }
}
