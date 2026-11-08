#pragma once
#include <vector>

struct PixelData {
	int width, height;
	std::vector<uint32_t> imageData;
};