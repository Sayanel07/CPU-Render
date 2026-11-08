#include "Light.h"

glm::mat4x4 Light::GetViewProj(const glm::vec3& _min, const glm::vec3& _max) const
{
    glm::vec3 dir = glm::normalize(lightDir);

    glm::vec3 up;
    if (glm::abs(dir.y) > 0.99f)
        up = glm::vec3(0, 0, 1);
    else
        up = glm::vec3(0, 1, 0);

    glm::vec3 center = (_min + _max) * 0.5f;
    float radius = glm::length(_max - _min) * 0.5f;
    glm::mat4 view = glm::lookAt(center - dir * radius, center, up);

    glm::vec3 mn(std::numeric_limits<float>::max());
    glm::vec3 mx(std::numeric_limits<float>::lowest());

    for (int i = 0; i < 8; ++i)
    {
        glm::vec3 corner(
            (i & 1) ? _max.x : _min.x,
            (i & 2) ? _max.y : _min.y,
            (i & 4) ? _max.z : _min.z
        );

        glm::vec3 p = glm::vec3(view * glm::vec4(corner, 1.f));
        mn = glm::min(mn, p);
        mx = glm::max(mx, p);
    }
    glm::mat4 proj = glm::ortho(mn.x, mx.x, mn.y, mx.y, -mx.z, -mn.z);

    return proj * view;
}

glm::mat4x4 Light::GetSpotViewProj(float _nearPlane) const
{
    glm::vec3 d = glm::normalize(lightDir);

    glm::vec3 up = (fabsf(d.y) > 0.99f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);

    glm::mat4 view = glm::lookAt(pos, pos + d, up);

    float fov = 2.0f * outerAngleRad;
    glm::mat4 proj = glm::perspective(fov, 1.0f, _nearPlane, range);

    return proj * view;
}

Frustum Light::GetFrustum(glm::mat4 _vp) const
{
    // Transpose so that t[i] gives row i of the original matrix
    glm::mat4 t = glm::transpose(_vp);
    const glm::vec4 row0 = t[0];
    const glm::vec4 row1 = t[1];
    const glm::vec4 row2 = t[2];
    const glm::vec4 row3 = t[3];

    glm::vec4 left = row3 + row0;
    glm::vec4 right = row3 - row0;
    glm::vec4 bottom = row3 + row1;
    glm::vec4 top = row3 - row1;
    glm::vec4 nearP = row3 + row2;
    glm::vec4 farP = row3 - row2;

    Frustum frustum;
    frustum.leftFace.CreatePlaneFromVector(left);
    frustum.rightFace.CreatePlaneFromVector(right);
    frustum.bottomFace.CreatePlaneFromVector(bottom);
    frustum.topFace.CreatePlaneFromVector(top);
    frustum.nearFace.CreatePlaneFromVector(nearP);
    frustum.farFace.CreatePlaneFromVector(farP);
    return frustum;
}

void Light::SetAngleD(float inner, float outer)
{
    innerAngleRad = glm::radians(inner);
    outerAngleRad = glm::radians(outer);

    cosInner = cosf(innerAngleRad);
    cosOuter = cosf(outerAngleRad);
}
