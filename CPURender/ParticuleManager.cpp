#include "ParticuleManager.h"
#include <random>

inline float RandomFloat(float _min, float _max)
{
	srand(time(NULL));
	static thread_local std::mt19937 gen{ std::random_device{}() };
	std::uniform_real_distribution<float> dist(_min, _max);
	return dist(gen);
}

vec3 RandomVector(vec3 _min, vec3 _max)
{
	float x = RandomFloat(_min.x, _max.x);
	float y = RandomFloat(_min.y, _max.y);
	float z = RandomFloat(_min.z, _max.z);
	return vec3{ x, y, z };
}

void ParticuleManager::Init()
{
	m_geo = GeoFactory::CreatePlane(0.1f, 0.1f);
	m_mesh = Mesh(m_geo);
	m_mat = Material();
	m_texture.LoadFromFile("Assets/Particules/light.png");
	m_mat.texture = std::make_shared<Texture>(m_texture);
	hwnd = Graphics::CreateBuffer(sizeof(Particule*));
}

void ParticuleManager::Update(float _deltatime)
{
	for(ParticuleEmitter& pE : partEmitters)
	{
		if (pE.type == Rain || pE.type == Snow)
			FallingField(pE, pE.type);
		else
			CreateParticule(pE, pE.transform, pE.type);

		for (Particule& p : pE.particules)
		{
			if (p.lifeCur <= p.lifeMax)
			{
				p.UpdatePart(cam, _deltatime);
			}
			else if (p.lifeCur >= p.lifeMax)
			{
				pE.particules.erase(pE.particules.begin());
			}
		}
	}
	
}

void ParticuleManager::SetCamera(Camera* _cam)
{
	cam = _cam;
}

void ParticuleManager::Clear()
{
	for (ParticuleEmitter pE : partEmitters)
	{
		pE.particules.clear();
	}
}

void ParticuleManager::CreateParticule(ParticuleEmitter& pE, Transform t, PartType _type = PartType::Default)
{
	Particule p = Particule();
	p.Init(t, _type);
	pE.particules.push_back(p);
}

void ParticuleManager::CreateParticulesPoint(vec3 _pos, PartType _type, float _size)
{
	ParticuleEmitter pE;
	pE.transform.setPosition(_pos);
	pE.type = _type;
	pE.cam = cam;
	pE.offsetField = _size;
	pE.nbPart = pE.offsetField * 1.f;
	pE.Init();
	partEmitters.push_back(pE);
}

void ParticuleManager::CreateRain(float _size)
{
	if (isSnowing)
		isSnowing = false;
	else if (isRaining)
		return;

	isRaining = true;
	CreateParticulesPoint(cam->transform.getPosition(), Rain, _size);
}

void ParticuleManager::CreateSnow(float _size)
{
	if (isRaining)
		isRaining = false;
	else if (isSnowing)
		return;

	isSnowing = true;
	CreateParticulesPoint(cam->transform.getPosition(), Snow, _size);
}

void ParticuleManager::ResetField()
{
	if(isRaining || isSnowing)
	{
		isRaining = false;
		isSnowing = false;

		for (ParticuleEmitter pE : partEmitters)
		{
			if (pE.type == Rain || pE.type == Snow)
			{
				pE.mat.texture.get()->Free();
				//partEmitters.erase(partEmitters.find())
			}
		}
	}
}

Transform ParticuleManager::GetTransform()
{
	return m_transform;
}

Mesh ParticuleManager::GetMesh()
{
	return m_mesh;
}

Material* ParticuleManager::GetMaterial()
{
	return &m_mat;
}

std::vector<ParticuleEmitter> ParticuleManager::GetEmitters()
{
	return partEmitters;
}

Camera* ParticuleManager::GetCam()
{
	return cam;
}

GPU_HANDLE ParticuleManager::GetHandle(int _id)
{
	Particule* p = partEmitters[_id].particules.data();
	Graphics::SetBuffer(hwnd, &p, sizeof(Particule*));
	return hwnd;
}


void ParticuleEmitter::Init()
{
	Texture t;

	switch (type)
	{
	case PartType::Smoke:
	{
		t.LoadFromFile("Assets/Particules/smoke.png");
		mat.texture = std::make_shared<Texture>(t);
		break;
	}
	case PartType::Leaves:
	{
		t.LoadFromFile("Assets/Particules/leaves.png");
		mat.texture = std::make_shared<Texture>(t);
		break;
	}
	case PartType::Sparks:
	{
		t.LoadFromFile("Assets/Particules/spark.png");
		mat.texture = std::make_shared<Texture>(t);
		break;
	}
	case PartType::Explosion:
	{
		t.LoadFromFile("Assets/Particules/spark.png");
		mat.texture = std::make_shared<Texture>(t);
		break;
	}
	case PartType::Rain:
	{
		t.LoadFromFile("Assets/Particules/water.png");
		mat.texture = std::make_shared<Texture>(t);
		break;
	}
	case PartType::Snow:
	{
		t.LoadFromFile("Assets/Particules/snow.png");
		mat.texture = std::make_shared<Texture>(t);
		break;
	}
	default:
	{
		t.LoadFromFile("Assets/PArticules/light.png");
		mat.texture = std::make_shared<Texture>(t);
		break;
	}
	}
}

void ParticuleManager::FallingField(ParticuleEmitter& pE, PartType _type)
{
	float camX = cam->transform.getPosition().x;
	float camZ = cam->transform.getPosition().z;

	float maxX = camX + pE.offsetField;	// X
	float minX = camX - pE.offsetField;
	float maxZ = camZ + pE.offsetField;	// Z
	float minZ = camZ - pE.offsetField;

	vec3 max = { maxX, 50, maxZ };
	vec3 min = { minX, 50, minZ };

	for (int i = 0; i < pE.nbPart; ++i)
	{
		vec3 pos = RandomVector(min, max);

		Transform transform;
		transform.setPosition(pos);

		CreateParticule(pE, transform, _type);
	}
}
