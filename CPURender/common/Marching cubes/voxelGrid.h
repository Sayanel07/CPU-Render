#pragma once
#include <vector>

struct VoxelGrid {
    VoxelGrid(int width, int height, int length) : 
    width(width), 
    height(height), 
    length(length), 
    grid(std::vector<float>(static_cast<size_t>(width)*height*length)) {}
        
    float read(int x, int y, int z) const {
        size_t index = x * (height * length) + y * length + z;
        return grid[index];
    }
        
    void write(float v, int x, int y, int z) {
        size_t index = x * (height * length) + y * length + z;
        grid[index] = v;
    }
        
    std::vector<float> grid;
    int width;
    int height;
    int length;
};
