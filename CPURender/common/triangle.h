#pragma once
#include <array>

#include "vertex.h"

class Triangle
{
public:
	std::array<VertexOut, 3> vertices;
	glm::vec2 min, max;
};

