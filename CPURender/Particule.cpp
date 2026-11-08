#include "Particule.h"
#include <random>

inline float RandomFloat(float min, float max)
{
	static thread_local std::mt19937 gen{ std::random_device{}() };
	std::uniform_real_distribution<float> dist(min, max);
	return dist(gen);
}

vec3 RandomVector()
{
	float z = RandomFloat(-1.0f, 1.0f);
	float a = RandomFloat(0.0f, 6.2831853f);
	float r = std::sqrt(1.0f - z * z);
	return vec3{ r * std::cos(a), r * std::sin(a), r * std::tan(z) };
}

void Particule::Init(Transform _transform, PartType _type)
{
	transform = _transform;
	type = _type;
	lifeCur = 0.0f;

	//dir = vec3{ 0, -G, 0 };
	dir = vec3{ 0, 0, 0 };

	switch (type)
	{
	case PartType::Default:
	{
		speed = 0.5f;
		lifeMax = 5.0f;
		dir += vec3{ RandomFloat(-1.0f, 1.0f), -1.0f, RandomFloat(-1.0f, 1.0f) };
		break;
	}

	case PartType::Smoke:
	{
		speed = RandomFloat(0.5f, 1.5f);
		lifeMax = RandomFloat(4.0f, 6.0f);
		dir += vec3{ RandomFloat(-0.3f, 0.3f), 1.0f, RandomFloat(-0.3f, 0.3f) };
		break;
	}

	case PartType::Leaves:
	{
		speed = RandomFloat(0.5f, 1.5f);
		lifeMax = RandomFloat(3.0f, 5.0f);
		dir += vec3{ RandomFloat(-1, 1), -1.0f, RandomFloat(-1, 1) };
		break;
	}

	case PartType::Explosion:
	{
		speed = RandomFloat(10.f, 15.f);
		lifeMax = 1.0f;
		dir += RandomVector();
		break;
	}

	case PartType::Sparks:
	{
		speed = RandomFloat(1.0f, 2.0f);
		lifeMax = 1.0f;
		dir += RandomVector();
		break;
	}

	case PartType::Rain:
	{
		speed = RandomFloat(50.0f, 75.0f);
		lifeMax = 5.0f;
		dir += vec3{ 0.f, -GRAVITY, 0.f };
		break;
	}

	case PartType::Snow:
	{
		speed = RandomFloat(5.0f, 10.0f);
		lifeMax = 10.0f;
		dir += vec3{ 0.f, -GRAVITY, 0.f };
		break;
	}
	}

	if (glm::length(dir) > 0.0001f)
		dir = glm::normalize(dir);
}

void Particule::UpdatePart(Camera* cam, float deltatime)
{
	transform.translate((dir * speed) * deltatime);
	
	float angle = 90.0f * 3.14159265f / 180.0f;
	float s = sin(angle * 0.5f);
	float c = cos(angle * 0.5f);

	// rotation autour de Z : (x, y, z, w) = (0, 0, s, c)
	glm::quat fix(0.0f, 0.0f, s, c);

	transform.setRotation(cam->transform.getRotation() * fix);

	if ((type == Rain || type == Snow) && transform.getPosition().y <= -1.0f)
	{
		lifeCur = lifeMax;
	}
	else
	{
		lifeCur += deltatime;
	}
}
