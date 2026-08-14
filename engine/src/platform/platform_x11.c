#include "platform_x11.h"

#include <X11/Xlib.h>

#include <stddef.h>

bool hth_platform_x11_observer_init(HTHPlatformX11Observer *observer,
                                    void *display)
{
    if (observer == NULL || display == NULL) {
        return false;
    }
    observer->display = display;
    return true;
}

bool hth_platform_x11_keymap_key_down(
    const unsigned char keymap[HTH_X11_KEYCODE_COUNT / 8],
    unsigned int keycode)
{
    unsigned int mask;

    if (keymap == NULL || keycode >= HTH_X11_KEYCODE_COUNT) {
        return false;
    }
    mask = 1U << (keycode % 8U);
    return (keymap[keycode / 8U] & mask) != 0U;
}

bool hth_platform_x11_query_keyboard(
    const HTHPlatformX11Observer *observer,
    bool keycodes_down[HTH_X11_KEYCODE_COUNT])
{
    unsigned char keymap[HTH_X11_KEYCODE_COUNT / 8];
    unsigned int keycode;

    if (observer == NULL || observer->display == NULL ||
        keycodes_down == NULL) {
        return false;
    }
    if (XQueryKeymap((Display *)observer->display, (char *)keymap) == 0) {
        return false;
    }
    for (keycode = 0; keycode < HTH_X11_KEYCODE_COUNT; ++keycode) {
        keycodes_down[keycode] = hth_platform_x11_keymap_key_down(
            keymap, keycode);
    }
    return true;
}
