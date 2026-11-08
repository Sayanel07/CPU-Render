#pragma once
#include <vector>
#include <glm/vec2.hpp>

#include "../../common/types.h"
#include "../../common/morton-table.h"
#include "../../common/helper.h"

struct Buffer2D
{
	Buffer2D() = default;
	
	Buffer2D(uint32 _width, uint32 _height, uint32 _formatSize, uint32 _mipLevels = 1);
	
	template<typename Format>
	Buffer2D(const std::vector<Format>& _dataArray, uint32 _width, uint32 _height, uint32 _mipLevels = 1);

	template<typename Format>
	void SetData(const std::vector<Format>& _dataArray, uint32 _width, uint32 _height, uint32 _mipLevels = 1);
	
	template<typename Format>
	void SetData(const Format* _dataArray, uint32 _width, uint32 _height, uint32 _mipLevels = 1);
	
    template<typename Format>
	void Set(const Format* _data, uint32 _x, uint32 _y, bool _shouldUpdate = true);

	template<typename Format>
	void Fill(const Format& value);

	template<typename Format>
	void Fill1(const Format& value);
	template<typename Format>
	void Fill3(const Format& value);
	
	uint32 GetFormatSize() const;
	
	uint32 GetMipCount() const { return static_cast<uint32>(levels.size()); }
	uint32 GetWidth(uint32 _mip = 0) const  { return levels[_mip].width; }
	uint32 GetHeight(uint32 _mip = 0) const { return levels[_mip].height; }	

	template<typename Format>
	Format Get(uint32 _x, uint32 _y, uint32 _mipLevel = 0) const;
	template<typename Format>
	Format Get(glm::uvec2 _xy, uint32 _mipLevel = 0) const;
    
    std::vector<char> GetDecodedData(uint32 _mipLevel = 0) const;

	void CopyFrom(const Buffer2D& _other);

private:
	struct MipLevel { uint32 width, height; size_t byteOffset; };
	std::vector<char> data = {};
	std::vector<MipLevel> levels;
	uint32 width = 0;
	uint32 height = 0;
	uint32 formatSize = 0;
	
	static uint32 FullMipCount(uint32 w, uint32 h) {
		return static_cast<uint32>(std::floor(std::log2(std::max(w, h))) + 1);
	}
	
	template<typename Format>
	void GenerateMipLevels();
	
	template<typename Format>
	void UpdateMipLevels(uint32 _x, uint32 _y);
	
	template<typename Format>
	Format Load(uint32 mip, uint32 x, uint32 y) const;
	
	template<typename Format>
	void Store(uint32 mip, uint32 x, uint32 y, const Format& v);
};

template<typename Format>
Buffer2D::Buffer2D(const std::vector<Format>& _dataArray, uint32 _width, uint32 _height, uint32 _mipLevels) {
    SetData(_dataArray, _width, _height, _mipLevels);
}
	
template<typename Format>
void Buffer2D::SetData(const std::vector<Format>& _dataArray, uint32 _width, uint32 _height, uint32 _mipLevels) {
	SetData(_dataArray.data(), _width, _height, _mipLevels);
}

template <typename Format>
void Buffer2D::SetData(const Format* _dataArray, uint32 _width, uint32 _height, uint32 _mipLevels) {	
    width = _width;
    height = _height;
    formatSize = sizeof(Format);
	
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
	
	for (uint32 y = 0; y < height; ++y)
		for (uint32 x = 0; x < width; ++x)
			Store<Format>(0, x, y, _dataArray[static_cast<uint64>(y) * width + x]);
	
	GenerateMipLevels<Format>();
}

template <typename Format>
void Buffer2D::Set(const Format* _dataArray, uint32 _x, uint32 _y, bool _shouldUpdate) {
	Store<Format>(0, _x, _y, *_dataArray);
	if (_shouldUpdate)
		UpdateMipLevels<Format>(_x,_y);
}

template<typename Format>
void Buffer2D::Fill(const Format& value) {
	Fill3(value);
}

template<typename Format>
void Buffer2D::Fill1(const Format& value)
{    
	assert(data.size() % sizeof(Format) == 0);

	Format* p = reinterpret_cast<Format*>(data.data());
    const int64_t count = static_cast<int64_t>(data.size() / sizeof(Format));
	constexpr int64_t tileSize = 16;

	#pragma omp parallel for schedule(static)
	for (int64_t tile = 0; tile < count; tile += tileSize)
	{
		const int64_t end = std::min(tile + tileSize, count);
		std::fill(p + tile, p + end, value);
	}
}

template<typename Format>
void Buffer2D::Fill3(const Format& value) {
	Format* p = reinterpret_cast<Format*>(data.data());
	const int64_t count = static_cast<int64_t>(data.size() / sizeof(Format));

	#pragma omp parallel for schedule(static)/* if(count > 100000)*/
    for (int64_t i = 0; i < count; ++i)
        p[i] = value;
}

template<typename Format>
Format Buffer2D::Get(uint32 _x, uint32 _y, uint32 _mipLevel) const {
	return Load<Format>(_mipLevel, _x, _y);
}

template<typename Format>
Format Buffer2D::Get(glm::uvec2 _xy, uint32 _mipLevel) const {
	return Get<Format>(_xy.x, _xy.y, _mipLevel);
}

template<typename Format>
Format Buffer2D::Load(uint32 mip, uint32 x, uint32 y) const {
	assert(mip < levels.size());
	const MipLevel& L = levels[mip];
	assert(sizeof(Format) == formatSize && x < L.width && y < L.height);
	Format r;
	memcpy(&r, data.data() + L.byteOffset + encodeMorton2DBMI(x, y) * formatSize, formatSize);
	return r;
}

template<typename Format>
void Buffer2D::Store(uint32 mip, uint32 x, uint32 y, const Format& v) {
	assert(mip < levels.size());
	MipLevel& L = levels[mip];
	assert(sizeof(Format) == formatSize && x < L.width && y < L.height);
	memcpy(data.data() + L.byteOffset + encodeMorton2DBMI(x, y) * formatSize, &v, formatSize);
}

template <typename Format>
void Buffer2D::GenerateMipLevels() {
	for (uint32 m = 1; m < levels.size(); ++m) {
		MipLevel& src = levels[m - 1]; // Previous MipLevel
		MipLevel& dst = levels[m]; // Actual MipLevel
		#pragma omp parallel for
		for (int32 y = 0; y < dst.height; ++y)
			for (int32 x = 0; x < dst.width; ++x) {
				uint32 x0 = x * 2;
				uint32 x1 = std::min(x0 + 1, src.width  - 1);
				uint32 y0 = y * 2;
				uint32 y1 = std::min(y0 + 1, src.height - 1);
				Format r0 = Load<Format>(m - 1, x0, y0);
				Format r1 = Load<Format>(m - 1, x1, y0);
				Format r2 = Load<Format>(m - 1, x0, y1);
				Format r3 = Load<Format>(m - 1, x1, y1);
				Store<Format>(m, x, y, Average4<Format>(r0,r1,r2,r3)); // Average of each value
			}
	}
}

template <typename Format>
void Buffer2D::UpdateMipLevels(uint32 _x, uint32 _y) {
	for (uint32 m = 1; m < levels.size(); ++m) {
		_x /= 2;
		_y /= 2;
		const MipLevel& src = levels[m - 1];
		const MipLevel& dst = levels[m];
		
		if (_x >= dst.width || _y >= dst.height) 
			break;

		uint32 x0 = _x * 2;
		uint32 x1 = std::min(x0 + 1, src.width  - 1);
		uint32 y0 = _y * 2;
		uint32 y1 = std::min(y0 + 1, src.height - 1);

		Format r0 = Load<Format>(m - 1, x0, y0);
		Format r1 = Load<Format>(m - 1, x1, y0);
		Format r2 = Load<Format>(m - 1, x0, y1);
		Format r3 = Load<Format>(m - 1, x1, y1);
		Store<Format>(m, _x, _y, Average4<Format>(r0,r1,r2,r3)); // Average of each value
	}
}
