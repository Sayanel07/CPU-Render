#pragma once
#include <glm/common.hpp>

#include "../../common/types.h"

enum BlendOP : uint8
{
    BLEND_OP_ADD = 0,
    BLEND_OP_SUBTRACT = 1,
    BLEND_OP_REV_SUBTRACT = 2,
    BLEND_OP_MIN = 3,
    BLEND_OP_MAX = 4
};

enum BlendFactor : uint8
{
    BLEND_ZERO = 0,
    BLEND_ONE = 1,
    BLEND_SRC_COLOR = 2,
    BLEND_INV_SRC_COLOR = 3,
    BLEND_SRC_ALPHA = 4,
    BLEND_INV_SRC_ALPHA = 5,
    BLEND_DEST_COLOR = 6,
    BLEND_INV_DEST_COLOR = 7,
    BLEND_DEST_ALPHA = 8,
    BLEND_INV_DEST_ALPHA = 9,
    // BLEND_SRC_ALPHA_SAT = 10,
    // BLEND_BLEND_FACTOR = 11,
    // BLEND_INV_BLEND_FACTOR = 12,
    // BLEND_SRC1_COLOR = 13,
    // BLEND_INV_SRC1_COLOR = 14,
    // BLEND_SRC1_ALPHA = 15,
    // BLEND_INV_SRC1_ALPHA = 16,
    // BLEND_ALPHA_FACTOR = 17,
    // BLEND_INV_ALPHA_FACTOR = 18
};

struct BlendDescriptor
{
    BlendOP colorOp = BLEND_OP_ADD;
    BlendFactor SrcBlend = BLEND_ONE;
    BlendFactor DestBlend = BLEND_ZERO;
    BlendOP alphaOp = BLEND_OP_ADD;
    BlendFactor SrcAlphaBlend = BLEND_ONE;
    BlendFactor DestAlphaBlend = BLEND_ZERO;
    
    Color Blend(Color _src, Color _dest) const
    {
        Color color = Operator(colorOp, GetFactor(SrcBlend, _src, _dest) * _src, GetFactor(DestBlend, _src, _dest) * _dest);
        float alpha = Operator(alphaOp, GetFactor(SrcAlphaBlend, _src, _dest) * _src, GetFactor(DestAlphaBlend, _src, _dest) * _dest).a;
        return {color.r, color.g, color.b, alpha};
    }
    
    static Color Operator(BlendOP op, Color _src, Color _dest)
    {
        switch (op)
        {
        case BLEND_OP_ADD:
            return _src + _dest;
        case BLEND_OP_SUBTRACT:
            return _src - _dest;
        case BLEND_OP_REV_SUBTRACT:
            return _dest - _src;
        case BLEND_OP_MIN:
            return glm::min(_src, _dest);
        case BLEND_OP_MAX:
            return glm::max(_src, _dest);
        }
        return _src + _dest;
    }

    static Color GetFactor(BlendFactor _factor, Color _src, Color _dest)
    {
        switch (_factor)
        {
        case BLEND_ONE:
            return glm::vec4{1};
        case BLEND_ZERO:
            return glm::vec4{0};
        case BLEND_SRC_COLOR:
            return _src;
        case BLEND_INV_SRC_COLOR:
            return glm::vec4{1} - _src;
        case BLEND_SRC_ALPHA:
            return glm::vec4{_src.a};
        case BLEND_INV_SRC_ALPHA:
            return glm::vec4{1 - _src.a};
        case BLEND_DEST_COLOR:
            return _dest;
        case BLEND_INV_DEST_COLOR:
            return glm::vec4{1} - _dest;
        case BLEND_DEST_ALPHA:
            return glm::vec4{_dest.a};
        case BLEND_INV_DEST_ALPHA:
            return glm::vec4{1 - _dest.a};
        }
        return glm::vec4{1};
    }
};
