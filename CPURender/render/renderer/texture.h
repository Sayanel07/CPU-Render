#pragma once
#include "../base/common.h"

struct Texture
{
	Texture() = default;
	
	void LoadFromFile(const char* fileName, uint32 _mipLevels = 1);
	
	GPU_HANDLE GetTextureBuffer() const;
	
	void Free();
private:
	GPU_HANDLE textureHandle = {};
	int32 width = 0;
	int32 height = 0;
};

