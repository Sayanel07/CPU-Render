#include "color.h"

namespace color {
    uint32 ToUint32(const Color& _color) {
        uint32 red = 255 * _color.r;
        uint32 blue = 255 *  _color.b;
        uint32 green = 255 * _color.g;
        uint32 alpha = 255 * _color.a;
        
        return (alpha << 24) | (red << 16) | (green << 8) | blue;
    }

    Color FromUint32(uint32 _color)
    {
        Color c;
        float s = 1.0f / 255.0f;
        c.b = ((_color >> 0) & 0xFF) * s;
        c.g = ((_color >> 8) & 0xFF) * s;
        c.r = ((_color >> 16) & 0xFF) * s;
        c.a = ((_color >> 24) & 0xFF) * s;
        return c;
    }

    Color FromR8G8B8A8(uint8 _r, uint8 _g, uint8 _b, uint8 a) {
        float s = 1.f / 255.f;
        return {_r * s, _g * s, _b * s, a * s };
    }
}
