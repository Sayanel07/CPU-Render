#include "Physics.h"
#include "transform.h"
#include "frustum.h"

AABB::AABB(const glm::vec3& _center, const glm::vec3& _halfExtents) :  max(_center + _halfExtents), min(_center - _halfExtents) {}

void AABB::translate(glm::vec3 _by) {
    min += _by;
    max += _by;
}

// Arvo’s method
void AABB::TransformAABB(const glm::mat4& m) {
    glm::vec3 outMin = glm::vec3(m[3]);           // start at the translation
    glm::vec3 outMax = glm::vec3(m[3]);
    for (int i = 0; i < 3; ++i) {                // output axis (row)
        for (int j = 0; j < 3; ++j) {            // input axis (column)
            float a = m[j][i] * min[j];         // glm is column-major: m[col][row]
            float b = m[j][i] * max[j];
            outMin[i] += glm::min(a, b);
            outMax[i] += glm::max(a, b);
        }
    }
    min = outMin;
    max = outMax;
}

// The isOnFrustum and isOnPlane function were done following the instructions from this site : https://learnopengl.com/Guest-Articles/2021/Scene/Frustum-Culling
bool Physics::isOnFrustum(AABB&& _aabb, const Frustum& frustum, Transform& transform) {
    return isOnFrustum(_aabb, frustum, transform);
}

bool Physics::isOnFrustum(const AABB& _aabb, const Frustum& frustum, Transform& transform) {
    glm::vec3 center = (_aabb.min + _aabb.max) * 0.5f;

    glm::mat4x4 worldMatrix = transform.getWorldMatrix();

    //Get global scale thanks to our transform
    glm::vec3 centerWorld = worldMatrix * glm::vec4(center, 1);

    glm::vec3 scale = transform.getScale();
    
    glm::vec3 halfExtents = (_aabb.max -center) * scale;

    glm::vec3 right = transform.getRight();
    glm::vec3 up = transform.getUp();
    glm::vec3 forward = transform.getForward();

    const float newIi = std::abs(right.x) * halfExtents.x +std::abs(up.x)* halfExtents.y + std::abs(forward.x) * halfExtents.z;

    const float newIj = std::abs(right.y) * halfExtents.x + std::abs(up.y) * halfExtents.y + std::abs(forward.y) * halfExtents.z;

    const float newIk = std::abs(right.z) * halfExtents.x + std::abs(up.z) * halfExtents.y + std::abs(forward.z) * halfExtents.z;

    //We not need to divide scale because it's based on the half extent of the AABB
    const AABB globalAABB(centerWorld, { newIi, newIj, newIk });

    return (isOnPlane(globalAABB, frustum.leftFace) &&
        isOnPlane(globalAABB, frustum.rightFace) &&
        isOnPlane(globalAABB, frustum.topFace) &&
        isOnPlane(globalAABB, frustum.bottomFace) &&
        isOnPlane(globalAABB, frustum.nearFace) &&
        isOnPlane(globalAABB, frustum.farFace));
}

bool Physics::isOnPlane(const AABB& _aabb, const Plane& _plane) {
    glm::vec3 center = (_aabb.max + _aabb.min) * 0.5f;
    glm::vec3 extents = _aabb.max - center;

    // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
    const float r = glm::dot(extents, glm::abs(_plane.normal));

    return -r <= _plane.getSignedDistanceToPlane(center);
}
