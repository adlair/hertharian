#include "platform.h"

#include <SDL3/SDL.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

struct HTHPlatform {
    SDL_Window *window;
    bool headless;
};

struct HTHPlatformGraphicsContext {
    SDL_GLContext handle;
};

static bool set_graphics_attribute(SDL_GLAttr attribute, int value)
{
    if (SDL_GL_SetAttribute(attribute, value)) {
        return true;
    }

    fprintf(stderr, "SDL OpenGL attribute configuration failed: %s\n",
            SDL_GetError());
    return false;
}

static bool prepare_graphics(void)
{
    return set_graphics_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) &&
           set_graphics_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) &&
           set_graphics_attribute(SDL_GL_CONTEXT_PROFILE_MASK,
                                  SDL_GL_CONTEXT_PROFILE_CORE) &&
           set_graphics_attribute(SDL_GL_DOUBLEBUFFER, 1) &&
           set_graphics_attribute(SDL_GL_DEPTH_SIZE, 24) &&
           set_graphics_attribute(SDL_GL_STENCIL_SIZE, 8);
}

static HTHKey translate_key(SDL_Scancode scancode)
{
    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
        return (HTHKey)(HTH_KEY_A + (scancode - SDL_SCANCODE_A));
    }
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
        return (HTHKey)(HTH_KEY_1 + (scancode - SDL_SCANCODE_1));
    }
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12) {
        return (HTHKey)(HTH_KEY_F1 + (scancode - SDL_SCANCODE_F1));
    }

    switch (scancode) {
    case SDL_SCANCODE_0: return HTH_KEY_0;
    case SDL_SCANCODE_ESCAPE: return HTH_KEY_ESCAPE;
    case SDL_SCANCODE_RETURN: return HTH_KEY_ENTER;
    case SDL_SCANCODE_SPACE: return HTH_KEY_SPACE;
    case SDL_SCANCODE_TAB: return HTH_KEY_TAB;
    case SDL_SCANCODE_BACKSPACE: return HTH_KEY_BACKSPACE;
    case SDL_SCANCODE_UP: return HTH_KEY_UP;
    case SDL_SCANCODE_DOWN: return HTH_KEY_DOWN;
    case SDL_SCANCODE_LEFT: return HTH_KEY_LEFT;
    case SDL_SCANCODE_RIGHT: return HTH_KEY_RIGHT;
    case SDL_SCANCODE_LSHIFT: return HTH_KEY_LSHIFT;
    case SDL_SCANCODE_RSHIFT: return HTH_KEY_RSHIFT;
    case SDL_SCANCODE_LCTRL: return HTH_KEY_LCTRL;
    case SDL_SCANCODE_RCTRL: return HTH_KEY_RCTRL;
    case SDL_SCANCODE_LALT: return HTH_KEY_LALT;
    case SDL_SCANCODE_RALT: return HTH_KEY_RALT;
    default: return HTH_KEY_UNKNOWN;
    }
}

static HTHMouseButton translate_mouse_button(uint8_t button)
{
    switch (button) {
    case SDL_BUTTON_LEFT: return HTH_MOUSE_LEFT;
    case SDL_BUTTON_MIDDLE: return HTH_MOUSE_MIDDLE;
    case SDL_BUTTON_RIGHT: return HTH_MOUSE_RIGHT;
    case SDL_BUTTON_X1: return HTH_MOUSE_X1;
    case SDL_BUTTON_X2: return HTH_MOUSE_X2;
    default: return HTH_MOUSE_UNKNOWN;
    }
}

bool hth_platform_init(HTHPlatform **platform, const HTHPlatformConfig *config)
{
    const char *video_driver;
    HTHPlatform *state;

    if (platform == NULL || config == NULL ||
        (!config->headless && (config->window_title == NULL ||
         config->window_width == 0 || config->window_width > INT_MAX ||
         config->window_height == 0 || config->window_height > INT_MAX))) {
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

    if (!SDL_Init(config->headless ? SDL_INIT_EVENTS : SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        free(state);
        return false;
    }

    state->headless = config->headless;
    if (config->headless) {
        puts("Headless mode.");
        *platform = state;
        puts("Platform initialized.");
        return true;
    }

    if (config->graphics_enabled && !prepare_graphics()) {
        SDL_Quit();
        free(state);
        return false;
    }

    state->window = SDL_CreateWindow(
        config->window_title,
        (int)config->window_width,
        (int)config->window_height,
        config->graphics_enabled
            ? SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
            : SDL_WINDOW_RESIZABLE
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

bool hth_platform_poll_event(HTHPlatform *platform, HTHPlatformEvent *event)
{
    SDL_Event native_event;
    double wheel_direction;

    if (platform == NULL || event == NULL) {
        return false;
    }

    while (SDL_PollEvent(&native_event)) {
        *event = (HTHPlatformEvent){0};
        event->timestamp_ns = native_event.common.timestamp;

        switch (native_event.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            event->type = HTH_PLATFORM_EVENT_QUIT;
            return true;
        case SDL_EVENT_KEY_DOWN:
            event->type = HTH_PLATFORM_EVENT_KEY_DOWN;
            event->data.keyboard.key = translate_key(native_event.key.scancode);
            return true;
        case SDL_EVENT_KEY_UP:
            event->type = HTH_PLATFORM_EVENT_KEY_UP;
            event->data.keyboard.key = translate_key(native_event.key.scancode);
            return true;
        case SDL_EVENT_MOUSE_MOTION:
            event->type = HTH_PLATFORM_EVENT_MOUSE_MOTION;
            event->data.motion.x = native_event.motion.x;
            event->data.motion.y = native_event.motion.y;
            event->data.motion.delta_x = native_event.motion.xrel;
            event->data.motion.delta_y = native_event.motion.yrel;
            return true;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            event->type = native_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                ? HTH_PLATFORM_EVENT_MOUSE_BUTTON_DOWN
                : HTH_PLATFORM_EVENT_MOUSE_BUTTON_UP;
            event->data.mouse_button.button =
                translate_mouse_button(native_event.button.button);
            event->data.mouse_button.x = native_event.button.x;
            event->data.mouse_button.y = native_event.button.y;
            return true;
        case SDL_EVENT_MOUSE_WHEEL:
            wheel_direction = native_event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED
                ? -1.0 : 1.0;
            event->type = HTH_PLATFORM_EVENT_MOUSE_WHEEL;
            event->data.wheel.x = (double)native_event.wheel.x * wheel_direction;
            event->data.wheel.y = (double)native_event.wheel.y * wheel_direction;
            return true;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            event->type = HTH_PLATFORM_EVENT_FOCUS_GAINED;
            return true;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            event->type = HTH_PLATFORM_EVENT_FOCUS_LOST;
            return true;
        case SDL_EVENT_WINDOW_RESIZED:
            event->type = HTH_PLATFORM_EVENT_WINDOW_RESIZED;
            event->data.window.width = native_event.window.data1 > 0
                ? (uint32_t)native_event.window.data1 : 0;
            event->data.window.height = native_event.window.data2 > 0
                ? (uint32_t)native_event.window.data2 : 0;
            return true;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            event->type = HTH_PLATFORM_EVENT_FRAMEBUFFER_RESIZED;
            event->data.window.width = native_event.window.data1 > 0
                ? (uint32_t)native_event.window.data1 : 0;
            event->data.window.height = native_event.window.data2 > 0
                ? (uint32_t)native_event.window.data2 : 0;
            return true;
        default:
            break;
        }
    }

    return false;
}

uint64_t hth_platform_time_counter(void)
{
    return SDL_GetPerformanceCounter();
}

uint64_t hth_platform_time_frequency(void)
{
    return SDL_GetPerformanceFrequency();
}

void hth_platform_sleep_ns(uint64_t nanoseconds)
{
    SDL_DelayNS(nanoseconds);
}

HTHPlatformGraphicsContext *hth_platform_graphics_create_context(
    HTHPlatform *platform)
{
    HTHPlatformGraphicsContext *context;

    if (platform == NULL || platform->window == NULL || platform->headless) {
        return NULL;
    }

    context = calloc(1, sizeof(*context));
    if (context == NULL) {
        fputs("Failed to allocate graphics context state.\n", stderr);
        return NULL;
    }

    context->handle = SDL_GL_CreateContext(platform->window);
    if (context->handle == NULL) {
        fprintf(stderr, "SDL OpenGL context creation failed: %s\n",
                SDL_GetError());
        free(context);
        return NULL;
    }

    return context;
}

void hth_platform_graphics_destroy_context(
    HTHPlatformGraphicsContext *context)
{
    if (context == NULL) {
        return;
    }

    if (!SDL_GL_DestroyContext(context->handle)) {
        fprintf(stderr, "SDL OpenGL context destruction failed: %s\n",
                SDL_GetError());
    }
    free(context);
}

bool hth_platform_graphics_make_current(
    HTHPlatform *platform, HTHPlatformGraphicsContext *context)
{
    if (platform == NULL || platform->window == NULL || context == NULL) {
        return false;
    }
    if (!SDL_GL_MakeCurrent(platform->window, context->handle)) {
        fprintf(stderr, "SDL OpenGL make-current failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

bool hth_platform_graphics_swap(HTHPlatform *platform)
{
    if (platform == NULL || platform->window == NULL) {
        return false;
    }
    if (!SDL_GL_SwapWindow(platform->window)) {
        fprintf(stderr, "SDL OpenGL presentation failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

bool hth_platform_graphics_set_swap_interval(int interval)
{
    if (!SDL_GL_SetSwapInterval(interval)) {
        fprintf(stderr, "SDL OpenGL swap interval configuration failed: %s\n",
                SDL_GetError());
        return false;
    }
    return true;
}

bool hth_platform_graphics_context_info(int *major, int *minor,
                                        bool *core_profile, int *double_buffer,
                                        int *depth_bits, int *stencil_bits)
{
    int profile;

    if (major == NULL || minor == NULL || core_profile == NULL ||
        double_buffer == NULL || depth_bits == NULL || stencil_bits == NULL ||
        !SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major) ||
        !SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor) ||
        !SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile) ||
        !SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, double_buffer) ||
        !SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, depth_bits) ||
        !SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, stencil_bits)) {
        fprintf(stderr, "SDL OpenGL context query failed: %s\n", SDL_GetError());
        return false;
    }

    *core_profile = (profile & SDL_GL_CONTEXT_PROFILE_CORE) != 0;
    return true;
}

bool hth_platform_framebuffer_size(HTHPlatform *platform,
                                   uint32_t *width, uint32_t *height)
{
    int pixel_width;
    int pixel_height;

    if (platform == NULL || platform->window == NULL ||
        width == NULL || height == NULL) {
        return false;
    }
    if (!SDL_GetWindowSizeInPixels(platform->window,
                                   &pixel_width, &pixel_height)) {
        fprintf(stderr, "SDL framebuffer size query failed: %s\n",
                SDL_GetError());
        return false;
    }
    if (pixel_width < 0 || pixel_height < 0) {
        return false;
    }

    *width = (uint32_t)pixel_width;
    *height = (uint32_t)pixel_height;
    return true;
}

HTHGraphicsProcedure hth_platform_graphics_get_proc_address(
    HTHPlatform *platform, const char *name)
{
    if (platform == NULL || platform->window == NULL || name == NULL) {
        return NULL;
    }
    return SDL_GL_GetProcAddress(name);
}
