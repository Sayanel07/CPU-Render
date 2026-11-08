#include "frustum.h"
#include <glm/glm.hpp>

float Plane::getSignedDistanceToPlane(const glm::vec3& point) const {
	return glm::dot(normal, point) + distance;
}

void Plane::CreatePlaneFromVector(const glm::vec4& v) {
	// Normalize using only the xyz (normal) part, and scale w to match
	float length = glm::length(glm::vec3(v));
	glm::vec4 p = v / length;

	normal = glm::vec3(p.x, p.y, p.z);
	distance = p.w;
}