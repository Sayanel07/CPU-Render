#include "mesh.h"

#include "graphics.h"

Mesh::Mesh(const Geometry& _geometry) {
    m_vertexBuffer = Graphics::CreateBuffer(_geometry.m_vertices.data(), sizeof(Vertex) * _geometry.m_vertices.size());
    m_indexBuffer = Graphics::CreateBuffer(_geometry.m_indices.data(), sizeof(uint32) * _geometry.m_indices.size());
    boundingBox.min = _geometry.m_min;
    boundingBox.max = _geometry.m_max;
}

void Mesh::SetGeometry(const Geometry& _geometry) {
    Graphics::SetBuffer(m_vertexBuffer, _geometry.m_vertices.data(), sizeof(Vertex) * _geometry.m_vertices.size());
    Graphics::SetBuffer(m_indexBuffer, _geometry.m_indices.data(), sizeof(uint32) * _geometry.m_indices.size());
    boundingBox.min = _geometry.m_min;
    boundingBox.max = _geometry.m_max;
}

GPU_HANDLE Mesh::GetVertexBuffer() const {
    return m_vertexBuffer;
}
GPU_HANDLE Mesh::GetIndexBuffer() const {
    return m_indexBuffer;
}

AABB Mesh::GetBoundingBox() const {
    return boundingBox;
}

void Mesh::Free()
{
    Graphics::FreeResource(m_vertexBuffer);
    m_vertexBuffer = {};
    Graphics::FreeResource(m_indexBuffer);
    m_indexBuffer = {};
}
