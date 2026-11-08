#pragma once
#include "Geometry.h"
#include "Vertex.h"

using namespace glm;

struct GeoFactory
{
private:
	static void AddTriangle(Geometry& geo, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, glm::vec3 normal);
	static void AddQuad(Geometry& geo, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 normal);

	static void FlipWinding(Geometry& geo);
	static void CalculNormals(Geometry& geo);

public:
	static Geometry CreateSphere(float _radius = 1.0f, int _sectors = 20, int _stacks = 32, bool _clockwise = true);
	static Geometry CreateCircle(float _radius = 1.0f, int _sliceCount = 20, bool _clockwise = true);
	static Geometry CreateCube(float _length = 1.0f, bool _clockwise = true);
	static Geometry CreateBox(float _length = 1.0f, float _depth = 1.0f, float _height = 1.0f, bool _clockwise = true);
	static Geometry CreateCone(float _radius = 1.0f, float _height = 2.0f, int _sliceCount = 32, bool _clockwise = true);
	static Geometry CreateCylinder(float _radius = 1.0f, float _height = 2.0f, int _sliceCount = 32, bool _clockwise = true);
	static Geometry CreatePyramid(float _length = 1.0f, float _height = 2.0f, bool _clockwise = true);
	static Geometry CreatePlane(float _length = 1.0f, float _depth = 1.0f, int _subdivision = 1, bool _clockwise = true);

};