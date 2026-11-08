#pragma once

#define EPSILON 0.0001f
#include <glm/vec3.hpp>

#include "transform.h"


struct Plane;
struct Frustum;

struct AABB  {
    glm::vec3 min = { -0.5f, -0.5f, -0.5f };
    glm::vec3 max = { 0.5f, 0.5f, 0.5f };

    AABB() = default;
    AABB(const glm::vec3& _center, const glm::vec3& _halfExtents);

    void translate(glm::vec3 _by);
    
    void TransformAABB(const glm::mat4& m);
};

struct Physics
{
    static bool isOnFrustum(AABB&& _aabb, const Frustum& frustum, Transform& transform);
    static bool isOnFrustum(const AABB& _aabb, const Frustum& frustum, Transform& transform);
    static bool isOnPlane(const AABB& _aabb, const Plane& _plane);
};