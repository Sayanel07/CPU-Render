#pragma once
#include "../../common/color.h"
#include "texture.h"
#include <memory>

#include "Renderer.h"

struct Material
{
	std::shared_ptr<Texture> texture;
	std::shared_ptr<Texture> normalMap;
	std::shared_ptr<Texture> metallicMap;
	std::shared_ptr<Texture> roughnessMap;
	Color color = Color{1.0f};
	RendererShader shader = RendererShader();
	bool _isTransparent = false;
    BlendDescriptor blendDescriptor;

	void SetWaterEffect(int width, int height);

private:
	GPU_HANDLE m_reflectionBuffer;
	GPU_HANDLE m_refractionBuffer;
	bool isWaterEffect = false;

};

