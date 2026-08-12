#include "platform.h"

#include <SDL3/SDL.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

struct HTHPlatform {
    SDL_Window *window;
};

bool hth_platform_init(HTHPlatform **platform, const HTHPlatformConfig *config)
{
    const char *video_driver;
    HTHPlatform *state;

    if (platform == NULL || config == NULL || config->window_title == NULL ||
        config->window_width == 0 || config->window_width > INT_MAX ||
        config->window_height == 0 || config->window_height > INT_MAX) {
        fputs("Invalid platform configuration.\n", stderr);
        return false;
    }

    *platform = NULL;
    puts("Initializing platform...");

    state = calloc(1, sizeof(*state));
    if (state == NULL) {
        fputs("Failed to allocate platform state.\n", stderr);
        return false;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        free(state);
        return false;
    }

    state->window = SDL_CreateWindow(
        config->window_title,
        (int)config->window_width,
        (int)config->window_height,
        0
    );
    if (state->window == NULL) {
        fprintf(stderr, "SDL window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        free(state);
        return false;
    }

    if (!SDL_ShowWindow(state->window)) {
        fprintf(stderr, "SDL window show failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(state->window);
        SDL_Quit();
        free(state);
        return false;
    }

    video_driver = SDL_GetCurrentVideoDriver();
    printf("SDL video driver: %s\n",
           video_driver != NULL ? video_driver : "unknown");

    *platform = state;
    puts("Platform initialized.");
    return true;
}

void hth_platform_shutdown(HTHPlatform *platform)
{
    if (platform == NULL) {
        return;
    }

    SDL_DestroyWindow(platform->window);
    SDL_Quit();
    free(platform);
    puts("Platform shutdown complete.");
}

bool hth_platform_pump_events(HTHPlatform *platform)
{
    SDL_Event event;

    if (platform == NULL) {
        return false;
    }

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT ||
            event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            return false;
        }
    }

    return true;
}

uint64_t hth_platform_time_counter(void)
{
    return SDL_GetPerformanceCounter();
}

uint64_t hth_platform_time_frequency(void)
{
    return SDL_GetPerformanceFrequency();
}

void hth_platform_sleep_ms(uint32_t milliseconds)
{
    SDL_Delay(milliseconds);
}
