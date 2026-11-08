#include "utils.h"


uint32_t Utils::RGB_255_ToInt(int r, int g, int b)
{
    uint32_t color = 0;

    color |= (uint32_t)b << 0;
    color |= (uint32_t)g << 8;
    color |= (uint32_t)r << 16;
    color |= (uint32_t)255 << 24;

    return color;
}

uint32_t Utils::RGB_255_ToInt(glm::vec3 color)
{
    return RGB_255_ToInt(color.x, color.y, color.z);
}

void Utils::DrawLine(glm::vec2 point1, glm::vec2 point2, FinalFrameBuffer& frameBufferOut, uint32_t color)
{
    //draw a ligne Bresenham's line 
    float dx = abs(point2.x - point1.x);
    float dy = abs(point2.y - point1.y);

    int sx = 1;
    if (point1.x > point2.x)
        sx = -1;

    int sy = 1;
    if (point1.y > point2.y)
        sy = -1;

    float err = dx - dy;
    bool cont = true;

    while (cont)
    {
        frameBufferOut.put_pixel(point1.x, point1.y, color);

        if (point1 == point2)
        {
            cont = false;
            break;
        }

        float e2 = 2 * err;

        if (e2 > -dy)
        {
            err -= dy;
            point1.x += sx;
        }

        if (e2 < dx)
        {
            err += dx;
            point1.y += sy;
        }
    }
}
