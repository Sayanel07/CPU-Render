#include "material.h"
#include "../../render/base/graphics.h"

void Material::SetWaterEffect(int width, int height)
{
	isWaterEffect = true;

	m_refractionBuffer = Graphics::CreateTextureBuffer(width, height, sizeof(uint32));
	m_reflectionBuffer = Graphics::CreateTextureBuffer(width, height, sizeof(uint32));
}
