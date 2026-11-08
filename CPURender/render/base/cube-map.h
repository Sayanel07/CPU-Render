#pragma once
#include <glm/vec3.hpp>

#include "buffer2D.h"
#include "common.h"

struct CubeMap
{
    CubeMap() = default;
	GPU_HANDLE map[6] = {};

	//right  -> +x
	//left   -> -x 
	//top    -> +y
	//bottom -> -y
	//back   -> +z
	//front  -> -z
};

