#ifndef HTH_PLATFORM_X11_H
#define HTH_PLATFORM_X11_H

#include <stdbool.h>

#define HTH_X11_KEYCODE_COUNT 256

typedef struct {
    void *display;
} HTHPlatformX11Observer;

bool hth_platform_x11_observer_init(HTHPlatformX11Observer *observer,
                                    void *display);
bool hth_platform_x11_keymap_key_down(
    const unsigned char keymap[HTH_X11_KEYCODE_COUNT / 8],
    unsigned int keycode);
bool hth_platform_x11_query_keyboard(
    const HTHPlatformX11Observer *observer,
    bool keycodes_down[HTH_X11_KEYCODE_COUNT]);

#endif
