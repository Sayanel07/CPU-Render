#include "GeoFactory.h"

void GeoFactory::AddTriangle(Geometry& geo, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, glm::vec3 normal)
{
	glm::vec3 points[3] = { p0, p1, p2 };
	
	glm::vec2 uv[3] = { uv0, uv1, uv2};
	
	int i = 0;
	for (glm::vec3 v : points)
	{
		Vertex vert;
		vert.pos = v;
		vert.normal = normal;
		vert.uv = uv[i];
		vert.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		geo.m_indices.push_back(static_cast<uint32_t>(geo.m_vertices.size()));
		geo.m_vertices.push_back(vert);
		i++;
	}
}

void GeoFactory::AddQuad(Geometry& geo, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 normal)
{
	glm::vec2 uv0 = { 1, 1 };
	glm::vec2 uv1 = { 1, 0 };
	glm::vec2 uv2 = { 0, 0 };
	glm::vec2 uv3 = { 0, 1 };

	AddTriangle(geo, p0, p1, p2, uv0, uv1, uv2, normal);
	AddTriangle(geo, p0, p2, p3, uv0, uv2, uv3, normal);
}

void GeoFactory::FlipWinding(Geometry& geo)
{
	for (size_t i = 0; i + 2 < geo.m_indices.size(); i += 3)
		std::swap(geo.m_indices[i + 1], geo.m_indices[i + 2]);
}

void GeoFactory::CalculNormals(Geometry& geo)
{
	for (Vertex& v : geo.m_vertices)
		v.normal = vec3(0);

	for (size_t i = 0; i + 2 < geo.m_indices.size(); i += 3)
	{
		Vertex& a = geo.m_vertices[geo.m_indices[i]];
		Vertex& b = geo.m_vertices[geo.m_indices[i + 1]];
		Vertex& c = geo.m_vertices[geo.m_indices[i + 2]];

		glm::vec3 n = glm::cross(b.pos - a.pos, c.pos - a.pos);
		a.normal += n;
		b.normal += n;
		c.normal += n;
	}

	for (Vertex& v : geo.m_vertices)
		v.normal = glm::normalize(v.normal);
}

Geometry GeoFactory::CreateSphere(float _radius, int _sectors, int _stacks, bool _clockwise)
{
	std::vector<Vertex> vertices;
	std::vector<uint32> indices;

	const float PI = 3.1415926f;
	const float PI2 = 6.283185307f;
	const float H_ANGLE = PI / 180 * 72;
	const float V_ANGLE = atanf(1.0f / 2);

	if (_stacks < 2) _stacks = 2;
	if (_sectors < 3) _sectors = 3;

	glm::vec4 _color = { 1.f, 1.f, 1.f, 1.f };

	Vertex topPole;
	topPole.pos = { 0.0f, _radius, 0.0f };
	topPole.normal = { 0, 1, 0 };
	topPole.uv = { 0.5f, 0.0f };
	topPole.color = _color;
	vertices.push_back(topPole);

	float phiStep = PI2 / _sectors;
	float thetaStep = PI / _stacks;

	for (int i = 1; i < _stacks; ++i)
	{
		float theta = i * thetaStep;


		for (int j = 0; j <= _sectors; ++j)
		{
			float phi = j * phiStep;

			Vertex v;

			v.pos.x = _radius * sinf(theta) * cosf(phi);
			v.pos.y = _radius * cosf(theta);
			v.pos.z = _radius * sinf(theta) * sinf(phi);
			v.normal = glm::normalize(v.pos);
			v.uv = { (float)j / _sectors, (float)i / _stacks };
			v.color = _color;

			vertices.push_back(v);
		}
	}

	Vertex bottomPole;
	bottomPole.pos = { 0.0f, -_radius, 0.0f };
	bottomPole.normal = { 0, -1, 0 };
	bottomPole.uv = { 0.5f, 1.0f };
	bottomPole.color = _color;
	vertices.push_back(bottomPole);

	for (std::uint32_t i = 1; i <= _sectors; ++i)
	{
		indices.push_back(0);
		indices.push_back(i + 1);
		indices.push_back(i);
	}
	std::uint32_t baseIndex = 1;
	std::uint32_t ringVertexCount = _sectors + 1;

	for (int i = 0; i < _stacks - 2; ++i)
	{
		for (int j = 0; j < _sectors; ++j)
		{
			std::uint32_t topLeft = baseIndex + i * ringVertexCount + j;
			std::uint32_t topRight = baseIndex + i * ringVertexCount + j + 1;
			std::uint32_t bottomLeft = baseIndex + (i + 1) * ringVertexCount + j;
			std::uint32_t bottomRight = baseIndex + (i + 1) * ringVertexCount + j + 1;


			indices.push_back(topLeft);
			indices.push_back(topRight);
			indices.push_back(bottomLeft);


			indices.push_back(bottomLeft);
			indices.push_back(topRight);
			indices.push_back(bottomRight);
		}
	}
	std::uint32_t southPoleIndex = static_cast<std::uint32_t>(vertices.size()) - 1;
	baseIndex = southPoleIndex - ringVertexCount;
	for (std::uint32_t i = 0; i < _sectors; ++i)
	{
		indices.push_back(southPoleIndex);
		indices.push_back(baseIndex + i);
		indices.push_back(baseIndex + i + 1);
	}

	Geometry geo(vertices, indices);
	CalculNormals(geo);
	if (!_clockwise) FlipWinding(geo);
	return geo;
}

Geometry GeoFactory::CreateCircle(float _radius, int _sliceCount, bool _clockwise)
{
	std::vector<Vertex> vertices;
	std::vector<uint32> indices;

	const float PI2 = 6.283185307f;
	
	_sliceCount = std::max(_sliceCount, 3);

	glm::vec4 _color = { 1.f, 1.f, 1.f, 1.f };

	vertices.reserve(_sliceCount * 3);

	Vertex center;
	center.pos = { 0.0f, 0.0f, 0.0f };
	center.uv = { 0.5f, 0.5f };
	center.color = _color;
	vertices.push_back(center);

	float step = PI2 / _sliceCount;
	float angle = 0.0f;

	for (int i = 0; i <= _sliceCount; i++)
	{
		Vertex v;
		v.pos.x = cosf(angle) * _radius;
		v.pos.y = 0.0f;
		v.pos.z = sinf(angle) * _radius;
		v.color = _color;
		v.uv = { cosf(angle) * 0.5f + 0.5f, sinf(angle) * 0.5f + 0.5f };
		vertices.push_back(v);

		angle += step;
	}
	for (std::uint32_t i = 1; i <= _sliceCount; ++i)
	{
		indices.push_back(0);
		indices.push_back(i + 1);
		indices.push_back(i);
	}

	Geometry geo(vertices, indices);
	CalculNormals(geo);
	if (!_clockwise) FlipWinding(geo);
	return geo;
}

Geometry GeoFactory::CreateCube(float _length, bool _clockwise)
{
	return CreateBox(_length, _length, _length, _clockwise);
}

Geometry GeoFactory::CreateBox(float _length, float _depth, float _height, bool _clockwise)
{
	Geometry geo;

	float l = std::abs(_length) * 0.5f;
	float d = std::abs(_depth) * 0.5f;
	float h = std::abs(_height) * 0.5f;

	glm::vec3 v0 = { l,  h,  d };
	glm::vec3 v1 = { l,  h, -d };
	glm::vec3 v2 = { -l, h, -d };
	glm::vec3 v3 = { -l, h,  d };
	glm::vec3 v4 = { l, -h,  d };
	glm::vec3 v5 = { l, -h, -d };
	glm::vec3 v6 = { -l, -h, -d };
	glm::vec3 v7 = { -l, -h,  d };

	if(_clockwise)
	{
		AddQuad(geo, v2, v3, v0, v1, { 0,  1,  0 });
		AddQuad(geo, v7, v6, v5, v4, { 0, -1,  0 });
		AddQuad(geo, v5, v1, v0, v4, { 1,  0,  0 });
		AddQuad(geo, v7, v3, v2, v6, { -1, 0,  0 });
		AddQuad(geo, v4, v0, v3, v7, { 0,  0,  1 });
		AddQuad(geo, v6, v2, v1, v5, { 0,  0, -1 });
	}
	else
	{
		AddQuad(geo, v2, v1, v0, v3, { 0,  1,  0 });
		AddQuad(geo, v7, v4, v5, v6, { 0, -1,  0 });
		AddQuad(geo, v5, v4, v0, v1, { 1,  0,  0 });
		AddQuad(geo, v7, v6, v2, v3, { -1, 0,  0 });
		AddQuad(geo, v4, v7, v3, v0, { 0,  0,  1 });
		AddQuad(geo, v6, v5, v1, v2, { 0,  0, -1 });
	}

	geo.FindMinMax();
	return geo;
}

Geometry GeoFactory::CreateCone(float _radius, float _height, int _sliceCount, bool _clockwise)
{
	const float PI = 3.1415926f;
	const float PI2 = 6.283185307f;
	
	std::vector<Vertex> vertices;
	std::vector<uint32> indices;

	glm::vec4 _color = { 1.f, 1.f, 1.f, 1.f };

	_sliceCount = std::max(_sliceCount, 3);
	float dTheta = PI2 / _sliceCount;

	Vertex center;
	center.pos = { 0, 0, 0 };
	center.uv = { 0.5f, 0.5f };
	center.color = _color;
	vertices.push_back(center);

	Vertex top;
	top.pos = { 0, _height, 0 };
	top.uv = { 0.5f, 0.5f };
	top.color = _color;
	vertices.push_back(top);

	float step = PI2 / _sliceCount;
	float angle = 0.0f;
	for (int i = 0; i < _sliceCount; i++)
	{
		float c = cosf(i * dTheta);
		float s = sinf(i * dTheta);
		
		Vertex v;
		v.pos.x = cosf(angle) * _radius;
		v.pos.y = 0.0f;
		v.pos.z = sinf(angle) * _radius;
		v.uv = { c * 0.5f + 0.5f, s * 0.5f + 0.5f };
		v.color = _color;
		vertices.push_back(v);

		angle += step;
	}
	for (int i = 0; i < _sliceCount; ++i)
	{
		uint32_t cur = 2 + i;
		uint32_t next = 2 + (i + 1) % _sliceCount;
		indices.push_back(0);	// base
		indices.push_back(cur);
		indices.push_back(next);
		indices.push_back(1);	// flanc
		indices.push_back(next);
		indices.push_back(cur);
	}

	Geometry geo(vertices, indices);
	CalculNormals(geo);
	if (!_clockwise) FlipWinding(geo);
	return geo;
}

Geometry GeoFactory::CreateCylinder(float _radius, float _height, int _sliceCount, bool _clockwise)
{
	std::vector<Vertex> vertices;
	std::vector<uint32> indices;

	const float PI2 = 6.283185307f;
	glm::vec4 _color = { 1.f, 1.f, 1.f, 1.f };

	_sliceCount = std::max(_sliceCount, 3);

	float halfHeight = _height * 0.5f;
	float dTheta = PI2 / _sliceCount;

	for (int i = 0; i <= _sliceCount; ++i)
	{
		float x = cosf(i * dTheta) * _radius;
		float z = sinf(i * dTheta) * _radius;

		Vertex vBottom;
		vBottom.pos = { x, -halfHeight, z };
		vBottom.uv = { (float)i / _sliceCount, 1.0f };
		vBottom.color = _color;
		vertices.push_back(vBottom);

		Vertex vTop;
		vTop.pos = { x, halfHeight, z };
		vTop.uv = { (float)i / _sliceCount, 0.0f };
		vTop.color = _color;
		vertices.push_back(vTop);
	}

	for (std::uint32_t i = 0; i < _sliceCount; ++i)
	{
		std::uint32_t bottomLeft = i * 2;
		std::uint32_t topLeft = i * 2 + 1;
		std::uint32_t bottomRight = (i + 1) * 2;
		std::uint32_t topRight = (i + 1) * 2 + 1;

		indices.push_back(bottomLeft);
		indices.push_back(topLeft);
		indices.push_back(topRight);
		indices.push_back(bottomLeft);
		indices.push_back(topRight);
		indices.push_back(bottomRight);
	}

	std::uint32_t baseIndexTop = static_cast<std::uint32_t>(vertices.size());

	Vertex topCenter;
	topCenter.pos = { 0.0f, halfHeight, 0.0f };
	topCenter.uv = { 0.5f, 0.5f };
	topCenter.color = _color;
	vertices.push_back(topCenter);
	for (int i = 0; i <= _sliceCount; ++i)
	{
		Vertex v;
		v.pos = { cosf(i * dTheta) * _radius, halfHeight, sinf(i * dTheta) * _radius };
		v.uv = { cosf(i * dTheta) * 0.5f + 0.5f, sinf(i * dTheta) * 0.5f + 0.5f };
		v.color = _color;
		vertices.push_back(v);
	}

	for (std::uint32_t i = 0; i < _sliceCount; ++i)
	{
		indices.push_back(baseIndexTop);
		indices.push_back(baseIndexTop + i + 2);
		indices.push_back(baseIndexTop + i + 1);
	}

	std::uint32_t baseIndexBottom = static_cast<std::uint32_t>(vertices.size());

	Vertex bottomCenter;
	bottomCenter.pos = { 0.0f, -halfHeight, 0.0f };
	bottomCenter.uv = { 0.5f, 0.5f };
	bottomCenter.color = _color;
	vertices.push_back(bottomCenter);

	for (int i = 0; i <= _sliceCount; ++i)
	{
		Vertex v;
		v.pos = { cosf(i * dTheta) * _radius, -halfHeight, sinf(i * dTheta) * _radius };
		v.uv = { cosf(i * dTheta) * 0.5f + 0.5f, sinf(i * dTheta) * 0.5f - 0.5f };
		v.color = _color;
		vertices.push_back(v);
	}
	for (std::uint32_t i = 0; i < _sliceCount; ++i)
	{
		indices.push_back(baseIndexBottom);
		indices.push_back(baseIndexBottom + i + 1);
		indices.push_back(baseIndexBottom + i + 2);
	}

	Geometry geo(vertices, indices);
	CalculNormals(geo);
	if (!_clockwise) FlipWinding(geo);
	return geo;
}

Geometry GeoFactory::CreatePyramid(float _length, float _height, bool _clockwise)
{
	return CreateCone(_length, _height, 4, _clockwise);
}

Geometry GeoFactory::CreatePlane(float _length, float _depth, int _subdivision, bool _clockwise)
{
	Geometry geo;

	_subdivision = std::max(1, _subdivision);

	float halfL = std::abs(_length) * 0.5f;
	float halfD = std::abs(_depth) * 0.5f;
	float stepX = std::abs(_length) / _subdivision;
	float stepZ = std::abs(_depth) / _subdivision;

	for (int ix = 0; ix < _subdivision; ++ix)
	{
		for (int iz = 0; iz < _subdivision; ++iz)
		{
			float x0 = -halfL + ix * stepX;
			float x1 = x0 + stepX;
			float z0 = -halfD + iz * stepZ;
			float z1 = z0 + stepZ;

			glm::vec3 v0 = { x1, 0, z1 };
			glm::vec3 v1 = { x1, 0, z0 };
			glm::vec3 v2 = { x0, 0, z0 };
			glm::vec3 v3 = { x0, 0, z1 };

			if (_clockwise)
				AddQuad(geo, v0, v1, v2, v3, { 0, 1, 0 });
			else
				AddQuad(geo, v0, v3, v2, v1, { 0, 1, 0 });
		}
	}

	geo.FindMinMax();
	return geo;
}
