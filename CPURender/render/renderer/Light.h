#pragma once
#include <glm/vec3.hpp>
#include "../../common/color.h"
#include "../../common/frustum.h"
#include "../../common/transform.h"

#define MAX_LIGHTS 3

enum LightType
{
	DIRECTIONAL,
	SPOT
};

class Light
{
private:

public:
	Color color = { 1, 1, 1, 1 };
	glm::vec3 lightDir = { 0, -1, 0 };
	float strength = 1.0f;
	LightType type = DIRECTIONAL;
	//for spot
	glm::vec3 pos = { 0.0f, 0.0f, 0.0f };
	float innerAngleRad;
	float outerAngleRad;
	float cosOuter;
	float cosInner;
	float range;

	glm::mat4x4 GetViewProj(const glm::vec3& _min, const glm::vec3& _max) const;
	glm::mat4x4 GetSpotViewProj(float _nearPlane) const;
	Frustum GetFrustum(glm::mat4 _vp) const;
	void SetAngleD(float inner, float outer);
};

struct LightPass
{
	Light lights[MAX_LIGHTS];
	uint32_t lightCount;
	glm::vec3 ambient = {0.1f,0.1f,0.1f};
	glm::mat4 lightViewProj[MAX_LIGHTS];
};
