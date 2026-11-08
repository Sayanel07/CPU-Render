#pragma once
#include "../base/mesh.h"
#include "material.h"
#include "../../common/transform.h"

struct Object
{
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
	Transform transform = Transform();
	bool castShadow = true;
};

struct WaterObject : public Object
{
	float absorption = 0.4f;
	float waveAmp = 5.0f;
	float ondulationReflect = 0.5f;
	float refractionStrength = 40.0f;
	bool usLightEffect = true;
	bool reflectSkybox = true;
};
