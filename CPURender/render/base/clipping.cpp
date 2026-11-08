#include "clipping.h"

Polygone Clipping::ClipOnPlane(PlaneType _plane, const Polygone& _polygone)
{
    Polygone p;

    if (_polygone.vertices.size() == 0) return p;

    VertexOut last = _polygone.vertices.back();
    

    for (const VertexOut& v : _polygone.vertices)
    {
        bool lastIsInside = IsInsidePlane(last.pos, _plane);
        bool vIsInside = IsInsidePlane(v.pos, _plane);

        if (vIsInside)
        {
            if (!lastIsInside)
            {
                //not fully inside the plane
                float t = Intersection(last.pos, v.pos, _plane);
                p.vertices.push_back(InterpolateVertex(last, v, t));
            }
            p.vertices.push_back(v);
        }
        else if (lastIsInside)
        {
            float t = Intersection(last.pos, v.pos, _plane);
            p.vertices.push_back(InterpolateVertex(last, v, t));
        }
        last = v;
    }

    return p;
}

bool Clipping::IsInsidePlane(glm::vec4 _pos, PlaneType _plane)
{
    return Distance(_pos, _plane) >= 0.0f;
}

float Clipping::Intersection(const glm::vec4& _v1, const glm::vec4& _v2, PlaneType _plane)
{
    float d1 = Distance(_v1, _plane);
    float d2 = Distance(_v2, _plane);
    return d1 / (d1 - d2);
}

VertexOut Clipping::InterpolateVertex(const VertexOut& _v1, const VertexOut& _v2, float f)
{
    VertexOut v;

    v.pos = glm::mix(_v1.pos, _v2.pos, f);
    v.color = glm::mix(_v1.color, _v2.color, f);
    v.uv = glm::mix(_v1.uv, _v2.uv, f);
    v.worldPos = glm::mix(_v1.worldPos, _v2.worldPos, f);
    v.tbn = _v1.tbn * (1.0f - f) + _v2.tbn * f;
    
    return v;
}

float Clipping::Distance(const glm::vec4& _v, PlaneType _plane)
{
    switch (_plane)
    {
    case LEFT:  return _v.w + _v.x;   // x >= -w
    case RIGHT: return _v.w - _v.x;   // x <=  w
    case DOWN:  return _v.w + _v.y;   // y >= -w
    case UP:    return _v.w - _v.y;   // y <=  w
    case NEAR:  return _v.w + _v.z;   // z >= -w
    case FAR:   return _v.w - _v.z;   // z <=  w
    case USER:  return glm::dot(s_userPlane, _v);
    }
    return 0.0f;

}

std::vector<Triangle> Clipping::ClipTriangle(Triangle _triangle)
{
    Polygone polygone;
    polygone.AddVerticesFromTriangle(_triangle);

    polygone = ClipOnPlane(RIGHT, polygone);
    polygone = ClipOnPlane(LEFT, polygone);
    polygone = ClipOnPlane(UP, polygone);
    polygone = ClipOnPlane(DOWN, polygone);
    polygone = ClipOnPlane(NEAR, polygone);
    polygone = ClipOnPlane(FAR, polygone);
    if (s_userPlaneEnabled)
        polygone = ClipOnPlane(USER, polygone);

    std::vector<Triangle> triangles;

    //construct the new triangle
    if (polygone.vertices.size() < 3) return triangles;

    for (int i = 1; i < polygone.vertices.size() - 1; i++)
    {
        Triangle triangle;
        triangle.vertices = {
            polygone.vertices[0],
            polygone.vertices[i],
            polygone.vertices[i + 1]
        };

        triangles.push_back(triangle);
    }

    return triangles;
}

void Clipping::SetUserClipPlane(const glm::vec4& _worldPlane, const glm::mat4& _viewProj)
{
    s_userPlane = glm::inverse(glm::transpose(_viewProj)) * _worldPlane;
    s_userPlaneEnabled = true;
}

void Clipping::DisableUserClipPlane()
{
    s_userPlaneEnabled = false;
}

void Clipping::EnableUserClipPlane()
{
    s_userPlaneEnabled = true;
}
