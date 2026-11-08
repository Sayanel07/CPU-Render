#include "buffer2D.h"

#include <algorithm>
#include "../../common/color.h"

Buffer2D::Buffer2D(uint32 _width, uint32 _height, uint32 _formatSize, uint32 _mipLevels) {
    width = _width;
    height = _height;
    formatSize =_formatSize;
	
    uint32 maxMipCount = FullMipCount(width, height);
    uint32 mipLevel = (_mipLevels == 0) ? maxMipCount : std::min(maxMipCount, _mipLevels);
    
    levels.clear();
    size_t total = 0;
    uint32 levelWidth = width;
    uint32 levelHeight = height;
    for (uint32 i = 0; i < mipLevel; ++i) {
        levels.push_back({ levelWidth, levelHeight, total });
        total += MortonBufferCount(levelWidth, levelHeight) * formatSize;
        levelWidth = std::max<uint32>(1, levelWidth / 2);
        levelHeight = std::max<uint32>(1, levelHeight / 2);
    }
    data.assign(total, 0);
}

uint32 Buffer2D::GetFormatSize() const {
	return formatSize;
}

std::vector<char> Buffer2D::GetDecodedData(uint32 _mipLevel) const {
    const MipLevel& level = levels[_mipLevel];
    std::vector<char> out(static_cast<uint64>(level.width) * level.height * formatSize);
    const char* start = data.data() + level.byteOffset;
    for (uint32_t y = 0; y < level.height; y++)
        for (uint32_t x = 0; x < level.width; x++)
            memcpy(out.data() + (static_cast<size_t>(y) * level.width + x) * formatSize, start + encodeMorton2DBMI(x, y) * formatSize, formatSize);
    return out;
}

void Buffer2D::CopyFrom(const Buffer2D& _other)
{
    assert(width == _other.width && height == _other.height);
    assert(formatSize == _other.formatSize);
    assert(data.size() == _other.data.size());

    memcpy(data.data(), _other.data.data(), data.size());
}
