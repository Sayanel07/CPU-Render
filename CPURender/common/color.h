#pragma once
#include "types.h"

namespace color {
    uint32 ToUint32(const Color& _color);
    Color FromUint32(uint32 _color);
    Color FromR8G8B8A8(uint8 _r, uint8 _g, uint8 _b, uint8 a = 255);
}


