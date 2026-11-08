#pragma once
#include "../../common/triangle.h"

enum PlaneType
{
	RIGHT,
	LEFT,
	UP,
	DOWN,
	NEAR,
	FAR,
	USER
};

struct Polygone
{
	std::vector<VertexOut> vertices;

	void AddVerticesFromTriangle(const Triangle& _triangle)
	{
		vertices.push_back(_triangle.vertices[0]);
		vertices.push_back(_triangle.vertices[1]);
		vertices.push_back(_triangle.vertices[2]);
	}
};

struct  Clipping
{
	static std::vector<Triangle> ClipTriangle(Triangle triangle);
	static void SetUserClipPlane(const glm::vec4& _worldPlane, const glm::mat4& _viewProj);
	static void DisableUserClipPlane();
	static void EnableUserClipPlane();
private:
	static inline glm::vec4 s_userPlane = glm::vec4(0.0f);
	static inline bool s_userPlaneEnabled = false;

	static Polygone ClipOnPlane(PlaneType _plane, const Polygone& _polygone);
	static bool IsInsidePlane(glm::vec4 _pos, PlaneType _plane);
	static float Intersection(const glm::vec4& _v1, const glm::vec4& _v2, PlaneType _plane);
	static VertexOut InterpolateVertex(const VertexOut& _v1, const VertexOut& _v2, float f);
	static float Distance(const glm::vec4& _v, PlaneType _plane);
};

