#pragma once
#include "../../common/geometry.h"
#include "../../common/Physics.h"
#include "common.h"

// For now basically only upload the geometry on the "GPU"
struct Mesh
{
    Mesh() = default;
    Mesh(const Geometry& _geometry);
    
    void SetGeometry(const Geometry& _geometry);
    
    GPU_HANDLE GetVertexBuffer() const;
    GPU_HANDLE GetIndexBuffer() const;

    AABB GetBoundingBox() const;
    
    void Free();
    
private:
    GPU_HANDLE m_vertexBuffer, m_indexBuffer;
    AABB boundingBox;
};

