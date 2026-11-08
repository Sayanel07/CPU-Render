#pragma once
#include "pch.h"
#include "render/renderer/Object.h"
#include "render/renderer/camera.h"

#define GRAVITY 9.81f

enum PartType
{
	Smoke,
	Leaves,
	Explosion,
	Sparks,
	Rain,
	Snow,
	Default
};

class Particule
{
	PartType type = PartType::Default;

	float speed;

	glm::vec3 dir;

public:
	Transform transform;

	float lifeMax;
	float lifeCur;

public:
	void Init(Transform transform, PartType _type = PartType::Default);
	void UpdatePart(Camera* cam, float deltatime);
};

