#pragma once
#include <algorithm>
#include <cassert>

#include "../../common/types.h"

#define MAX_ENTRIES 16
#define PIXEL_DISCARD 0.0f

enum Format : uint8
{
    NIL = 0,
    
    BUFFER1D = 0b000000001,
    
    BUFFER2D = 0b00000010,
    
    CUBEMAP = 0b00000100,
    
    SHADER = 0b00001000,
    PIXEL_SHADER = 0b00011000,
    VERTEX_SHADER = 0b00101000,
    COMPUTE_SHADER = 0b01001000,
};

struct ShaderInput
{
    ShaderInput() = default;
    ShaderInput(Format _format, uint32 _size = -1) : format(_format), size(_size) {}
    // size doesn't matter for BUFFER2D
    Format format = NIL;
    uint32 size = -1; // -1 means a BUFFER1D can be of any size
};

struct GPU_HANDLE
{
    GPU_HANDLE() = default;
    
    GPU_HANDLE(const GPU_HANDLE& hwnd) = default;
    GPU_HANDLE(GPU_HANDLE&& hwnd) noexcept : value(hwnd.value) {}
    
    GPU_HANDLE& operator=(const GPU_HANDLE& hwnd) = default;
    
    GPU_HANDLE& operator=(GPU_HANDLE&& hwnd) noexcept {
        value = hwnd.value;
        return *this;
    }
    
    bool operator==(const GPU_HANDLE& hwnd) const noexcept {
        return value == hwnd.value;
    }
    
    bool operator==(GPU_HANDLE&& hwnd) const noexcept {
        return value == std::move(hwnd).value;
    }
    
    bool operator!=(const GPU_HANDLE& hwnd) const noexcept {
        return value != hwnd.value;
    }
    
    bool operator!=(GPU_HANDLE&& hwnd) const noexcept {
        return value != std::move(hwnd).value;
    }

    bool IsValid() const {
        return value != 0x00000000;
    }
    
    uint8 GetFormat() const {
        return static_cast<uint8>(value >> 24);
    }
    
private:
    // 24 low bits: resource index
    // 8 high bits: format
    uint32 value = 0x00000000;

    explicit GPU_HANDLE(uint32 index, uint8 format) : value((static_cast<uint32>(format) << 24) | (index & 0x00FFFFFF)) {
        assert(index <= 0x00FFFFFF);
    }

    uint32 GetIndex() const {
        return value & 0x00FFFFFF;
    }

    friend class Graphics;
};