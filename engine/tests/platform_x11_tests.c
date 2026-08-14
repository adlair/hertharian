#include "platform_x11.h"

#include <assert.h>
#include <stddef.h>

int main(void)
{
    unsigned char keymap[HTH_X11_KEYCODE_COUNT / 8] = {0};

    keymap[25U / 8U] = (unsigned char)(1U << (25U % 8U));
    keymap[255U / 8U] |= (unsigned char)(1U << (255U % 8U));
    assert(hth_platform_x11_keymap_key_down(keymap, 25U));
    assert(hth_platform_x11_keymap_key_down(keymap, 255U));
    assert(!hth_platform_x11_keymap_key_down(keymap, 24U));
    assert(!hth_platform_x11_keymap_key_down(keymap, 256U));
    assert(!hth_platform_x11_keymap_key_down(NULL, 25U));
    return 0;
}
