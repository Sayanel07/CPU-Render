#pragma once
// External dependencies
#include <glm/glm.hpp>


struct Plane {
	glm::vec3 normal{0.0f, 1.0f, 0.0f};
	float distance = 0.0f;

	float getSignedDistanceToPlane(const glm::vec3& point) const;
	void CreatePlaneFromVector(const glm::vec4& v);
};

struct Frustum
{
	Plane topFace;
	Plane bottomFace;

	Plane rightFace;
	Plane leftFace;

	Plane farFace;
	Plane nearFace;
};