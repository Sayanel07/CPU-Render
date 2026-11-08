#pragma once
#include <glm/vec3.hpp>
#include <glm/glm.hpp>

struct Vertex
{
	glm::vec3 pos = { 0.0f, 0.0f, 0.0f };
    glm::vec4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
	glm::vec2 uv = { 0.0f, 0.0f };
	glm::vec3 normal = { 0.0f, 0.0f, 0.0f };
    glm::vec3 tangent = {0.0f,0.0,0.0};
    glm::vec3 biTangent = {0.0f,0.0f,0.0f};
};

struct VertexOut
{
	glm::vec4 pos = { 0.0f, 0.0f, 0.0f, 0.0f };
	glm::vec4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
	glm::vec2 uv = { 0.0f, 0.0f };
	glm::vec3 worldPos = { 0.0f, 0.0f, 0.0f };
    glm::mat3 tbn = {1.0f};
	glm::vec2 duvX = {0.0f, 0.0f};
	glm::vec2 duvY = {0.0f, 0.0f};
};

