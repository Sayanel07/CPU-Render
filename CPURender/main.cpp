#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "engine/src/engine.h"
//Students cannot change anything in this file

/* Main function */
int main(void) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Event event;

    const double target_frame = 1.0 / 60.0;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    window = SDL_CreateWindow(
        "Framebuffer",
        WIDTH * 2,
        HEIGHT * 2,
        SDL_WINDOW_RESIZABLE
    );

    SDL_SetWindowRelativeMouseMode(window, true); //added

    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    renderer = SDL_CreateRenderer(
        window,
        NULL
    );

    if (renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());

        SDL_DestroyWindow(window);
        SDL_Quit();

        return EXIT_FAILURE;
    }

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_XRGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH,
        HEIGHT
    );

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    if (texture == NULL) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();

        return EXIT_FAILURE;
    }

    Engine engine;
    engine.window = window;
    engine.init(WIDTH, HEIGHT);
    bool isRunning = true;
    uint32_t frame = 1;
    std::shared_ptr<FinalFrameBuffer> frameBuffer = std::make_shared<FinalFrameBuffer>();
    

    /* Frame loop */
    while (isRunning) {
        uint64_t start = SDL_GetPerformanceCounter();

        /* Poll events */
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                isRunning = false;
            }
            engine.handleEvent(event);
        }

        engine.renderFrame(*frameBuffer);

        /* Copy contents of the framebuffer to our SDL texture */
        SDL_UpdateTexture(
            texture,
            NULL,
            &frameBuffer->colorData,
            WIDTH * sizeof(uint32_t)
        );

        /* Render texture and display the renderer */
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        uint64_t end = SDL_GetPerformanceCounter();

        double elapsed = (double)(end - start) / (double)SDL_GetPerformanceFrequency();

        /* Cap the framerate to 60 FPS (16 milliseconds each frame) */
        if (elapsed < target_frame) {
            SDL_Delay((uint32_t)((target_frame - elapsed) * 1000.0));
        }

        frame++;
    }

    engine.destroy();
    /* Free all the SDL resources we created */
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}