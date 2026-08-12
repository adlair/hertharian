#include "event.h"
#include "hth_input.h"
#include "input_internal.h"

#include <assert.h>
#include <stddef.h>

static void test_keyboard_edges(void)
{
    HTHInput *input = hth_input_create();
    HTHPlatformEvent event = {0};

    assert(input != NULL);
    hth_input_begin_frame(input);
    event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
    event.data.keyboard.key = HTH_KEY_W;
    hth_input_handle_event(input, &event);
    assert(hth_input_key_down(input, HTH_KEY_W));
    assert(hth_input_key_pressed(input, HTH_KEY_W));
    assert(!hth_input_key_released(input, HTH_KEY_W));

    hth_input_begin_frame(input);
    assert(hth_input_key_down(input, HTH_KEY_W));
    assert(!hth_input_key_pressed(input, HTH_KEY_W));

    event.type = HTH_PLATFORM_EVENT_KEY_UP;
    hth_input_handle_event(input, &event);
    assert(!hth_input_key_down(input, HTH_KEY_W));
    assert(hth_input_key_released(input, HTH_KEY_W));
    hth_input_destroy(input);
}

static void test_mouse_transients(void)
{
    HTHInput *input = hth_input_create();
    HTHPlatformEvent event = {0};
    double x;
    double y;

    assert(input != NULL);
    hth_input_begin_frame(input);
    event.type = HTH_PLATFORM_EVENT_MOUSE_MOTION;
    event.data.motion.x = 100.0;
    event.data.motion.y = 50.0;
    event.data.motion.delta_x = 4.0;
    event.data.motion.delta_y = -2.0;
    hth_input_handle_event(input, &event);
    event.type = HTH_PLATFORM_EVENT_MOUSE_WHEEL;
    event.data.wheel.x = 1.0;
    event.data.wheel.y = -1.0;
    hth_input_handle_event(input, &event);

    hth_input_mouse_delta(input, &x, &y);
    assert(x == 4.0 && y == -2.0);
    hth_input_mouse_wheel(input, &x, &y);
    assert(x == 1.0 && y == -1.0);

    hth_input_begin_frame(input);
    hth_input_mouse_delta(input, &x, &y);
    assert(x == 0.0 && y == 0.0);
    hth_input_mouse_wheel(input, &x, &y);
    assert(x == 0.0 && y == 0.0);
    hth_input_mouse_position(input, &x, &y);
    assert(x == 100.0 && y == 50.0);
    hth_input_destroy(input);
}

static void test_focus_loss(void)
{
    HTHInput *input = hth_input_create();
    HTHPlatformEvent event = {0};

    assert(input != NULL);
    hth_input_begin_frame(input);
    event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
    event.data.keyboard.key = HTH_KEY_A;
    hth_input_handle_event(input, &event);
    event.type = HTH_PLATFORM_EVENT_MOUSE_BUTTON_DOWN;
    event.data.mouse_button.button = HTH_MOUSE_LEFT;
    hth_input_handle_event(input, &event);

    event.type = HTH_PLATFORM_EVENT_FOCUS_LOST;
    hth_input_handle_event(input, &event);
    assert(!hth_input_key_down(input, HTH_KEY_A));
    assert(hth_input_key_released(input, HTH_KEY_A));
    assert(!hth_input_mouse_button_down(input, HTH_MOUSE_LEFT));
    assert(hth_input_mouse_button_released(input, HTH_MOUSE_LEFT));
    hth_input_destroy(input);
}

int main(void)
{
    test_keyboard_edges();
    test_mouse_transients();
    test_focus_loss();
    return 0;
}
