#include "texture.h"

#include <stdexcept>

#include "../../common/stb_image.h"
#include "../base/graphics.h"

void Texture::LoadFromFile(const char* fileName, uint32 _mipLevels) {
    int nbrOfChannel;
    unsigned char* data = stbi_load(fileName, &width, &height, &nbrOfChannel, 4);
    if (!data)
        throw std::runtime_error("Failed to load texture from file");
    
    const uint64 byteCount = static_cast<uint64>(width) * height * 4;
    for (uint64 i = 0; i < byteCount; i += 4)
        std::swap(data[i + 0], data[i + 2]);
    
    textureHandle = Graphics::CreateTextureBuffer();
    Graphics::SetTextureBuffer(textureHandle, reinterpret_cast<const uint32*>(data), width, height, _mipLevels);
    stbi_image_free(data);
}

GPU_HANDLE Texture::GetTextureBuffer() const {
    return textureHandle;
}

void Texture::Free() {
    Graphics::FreeResource(textureHandle);
    textureHandle = {};
}
