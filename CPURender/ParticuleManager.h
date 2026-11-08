#pragma once
#include "pch.h"
#include "render/renderer/Object.h"
#include "render/renderer/camera.h"
#include "render/base/mesh.h"
#include "render/renderer/material.h"
#include "render/renderer/Renderer.h"
#include "Particule.h"
#include "render/renderer/texture.h"

struct ParticuleEmitter
{
	Transform transform;
	Material mat;
	PartType type = PartType::Default;
	std::vector<Particule> particules;

	Camera* cam;

	float offsetField;
	float nbPart;

	void Init();
	void SetCamera(Camera* _cam);
};

class ParticuleManager
{
private:
	std::vector<ParticuleEmitter> partEmitters;
	GPU_HANDLE hwnd;

	Transform m_transform;
	Geometry m_geo;
	Mesh m_mesh;
	Material m_mat;
	Texture m_texture;
	
	float speed = 0.1f;

	Camera* cam;

	bool isRaining = false;
	bool isSnowing = false;

public:

	void Init();
	void Update(float _deltatime);

	void SetCamera(Camera* _cam);
	void Clear();
	void CreateParticule(ParticuleEmitter& pE, Transform t, PartType _type);
	void CreateParticulesPoint(vec3 _pos, PartType _type = PartType::Default, float _size = 1.0f);
	void CreateRain(float _size = 32.0f);
	void CreateSnow(float _size = 32.0f);
	void FallingField(ParticuleEmitter& pE, PartType _type);
	void ResetField();


	Transform GetTransform();
	Mesh GetMesh();
	Material* GetMaterial();
	std::vector<ParticuleEmitter> GetEmitters();
	Camera* GetCam();
	GPU_HANDLE GetHandle(int _id);
};

