#ifndef HTH_EVENT_H
#define HTH_EVENT_H

#include "hth_input.h"

#include <stdint.h>

typedef enum {
    HTH_PLATFORM_EVENT_NONE = 0,
    HTH_PLATFORM_EVENT_QUIT,
    HTH_PLATFORM_EVENT_KEY_DOWN,
    HTH_PLATFORM_EVENT_KEY_UP,
    HTH_PLATFORM_EVENT_MOUSE_MOTION,
    HTH_PLATFORM_EVENT_MOUSE_BUTTON_DOWN,
    HTH_PLATFORM_EVENT_MOUSE_BUTTON_UP,
    HTH_PLATFORM_EVENT_MOUSE_WHEEL,
    HTH_PLATFORM_EVENT_FOCUS_GAINED,
    HTH_PLATFORM_EVENT_FOCUS_LOST,
    HTH_PLATFORM_EVENT_WINDOW_RESIZED,
    HTH_PLATFORM_EVENT_FRAMEBUFFER_RESIZED
} HTHPlatformEventType;

typedef struct {
    HTHPlatformEventType type;
    uint64_t timestamp_ns;
    union {
        struct { HTHKey key; bool repeat; } keyboard;
        struct { double x; double y; double delta_x; double delta_y; } motion;
        struct { HTHMouseButton button; double x; double y; } mouse_button;
        struct { double x; double y; } wheel;
        struct { uint32_t width; uint32_t height; } window;
    } data;
} HTHPlatformEvent;

#endif
