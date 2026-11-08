#pragma once
#include <stdint.h>
#include <stddef.h>

//Students cannot change anything in this file

#define WIDTH 320 * 2
#define HEIGHT 240 * 2


struct FinalFrameBuffer {

    
    /*
    * The pixels of highest weigth are ignored (alpha)
    * then it is red
    * then it is green
    * the values from 0-255 are the blue component
    */
    uint32_t colorData[WIDTH * HEIGHT];

    /* Draw a pixel in the framebuffer at 2D coordinates x and y */
    void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
        if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
            return;
        }
        colorData[y * WIDTH + x] = color;
    }

    /* Clear the entire framebuffer with a unique color */
    void clear(uint32_t color) {
        for (uint32_t i = 0; i < WIDTH * HEIGHT; i++) {
            colorData[i] = color;
        }
    }
};