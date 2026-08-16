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

static void test_repeat_restores_held_state_without_pressed_edge(void)
{
    HTHInput *input = hth_input_create();
    HTHPlatformEvent event = {0};

    assert(input != NULL);
    hth_input_begin_frame(input);
    event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
    event.data.keyboard.key = HTH_KEY_W;
    hth_input_handle_event(input, &event);
    assert(hth_input_key_down(input, HTH_KEY_W));

    event.type = HTH_PLATFORM_EVENT_FOCUS_LOST;
    hth_input_handle_event(input, &event);
    assert(!hth_input_key_down(input, HTH_KEY_W));

    hth_input_begin_frame(input);
    event.type = HTH_PLATFORM_EVENT_FOCUS_GAINED;
    hth_input_handle_event(input, &event);
    event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
    event.data.keyboard.repeat = true;
    hth_input_handle_event(input, &event);
    assert(hth_input_key_down(input, HTH_KEY_W));
    assert(!hth_input_key_pressed(input, HTH_KEY_W));

    event.type = HTH_PLATFORM_EVENT_KEY_UP;
    hth_input_handle_event(input, &event);
    hth_input_begin_frame(input);
    event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
    event.data.keyboard.repeat = false;
    hth_input_handle_event(input, &event);
    assert(hth_input_key_down(input, HTH_KEY_W));
    assert(hth_input_key_pressed(input, HTH_KEY_W));
    hth_input_destroy(input);
}

static void test_reconciled_release_then_repeat_recovers_movement_keys(void)
{
    static const HTHKey movement_keys[] = {
        HTH_KEY_W, HTH_KEY_A, HTH_KEY_S, HTH_KEY_D
    };
    HTHInput *input = hth_input_create();
    HTHPlatformEvent event = {0};
    size_t index;

    assert(input != NULL);
    for (index = 0U; index < sizeof(movement_keys) /
                               sizeof(movement_keys[0]); ++index) {
        hth_input_begin_frame(input);
        event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
        event.data.keyboard.key = movement_keys[index];
        event.data.keyboard.repeat = false;
        hth_input_handle_event(input, &event);
        assert(hth_input_key_down(input, movement_keys[index]));

        event.type = HTH_PLATFORM_EVENT_KEY_UP; /* Reconciled release. */
        hth_input_handle_event(input, &event);
        assert(!hth_input_key_down(input, movement_keys[index]));

        hth_input_begin_frame(input);
        event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
        event.data.keyboard.repeat = true;
        hth_input_handle_event(input, &event);
        assert(hth_input_key_down(input, movement_keys[index]));
        assert(!hth_input_key_pressed(input, movement_keys[index]));

        event.type = HTH_PLATFORM_EVENT_KEY_UP;
        hth_input_handle_event(input, &event);
    }
    hth_input_destroy(input);
}

static void test_repeat_recovery_multi_key_and_rapid_edges(void)
{
    HTHInput *input = hth_input_create();
    HTHPlatformEvent event = {0};

    assert(input != NULL);
    hth_input_begin_frame(input);
    event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
    event.data.keyboard.repeat = false;
    event.data.keyboard.key = HTH_KEY_W;
    hth_input_handle_event(input, &event);
    event.data.keyboard.key = HTH_KEY_D;
    hth_input_handle_event(input, &event);

    event.type = HTH_PLATFORM_EVENT_KEY_UP;
    event.data.keyboard.key = HTH_KEY_W;
    hth_input_handle_event(input, &event);
    event.data.keyboard.key = HTH_KEY_D;
    hth_input_handle_event(input, &event);
    hth_input_begin_frame(input);

    event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
    event.data.keyboard.repeat = true;
    event.data.keyboard.key = HTH_KEY_W;
    hth_input_handle_event(input, &event);
    event.data.keyboard.key = HTH_KEY_D;
    hth_input_handle_event(input, &event);
    assert(hth_input_key_down(input, HTH_KEY_W));
    assert(hth_input_key_down(input, HTH_KEY_D));
    assert(!hth_input_key_pressed(input, HTH_KEY_W));
    assert(!hth_input_key_pressed(input, HTH_KEY_D));

    event.type = HTH_PLATFORM_EVENT_KEY_UP;
    event.data.keyboard.key = HTH_KEY_W;
    hth_input_handle_event(input, &event);
    assert(!hth_input_key_down(input, HTH_KEY_W));
    event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
    event.data.keyboard.repeat = false;
    hth_input_handle_event(input, &event);
    assert(hth_input_key_down(input, HTH_KEY_W));
    assert(hth_input_key_pressed(input, HTH_KEY_W));
    hth_input_destroy(input);
}

static void test_five_repeat_recovery_cycles_and_cross_key(void)
{
    HTHInput *input = hth_input_create();
    HTHPlatformEvent event = {0};
    unsigned int cycle;

    assert(input != NULL);
    for (cycle = 0U; cycle < 5U; ++cycle) {
        hth_input_begin_frame(input);
        event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
        event.data.keyboard.key = HTH_KEY_W;
        event.data.keyboard.repeat = false;
        hth_input_handle_event(input, &event);
        assert(hth_input_key_down(input, HTH_KEY_W));

        event.type = HTH_PLATFORM_EVENT_KEY_UP; /* Reconciled release. */
        hth_input_handle_event(input, &event);
        assert(!hth_input_key_down(input, HTH_KEY_W));

        hth_input_begin_frame(input);
        event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
        event.data.keyboard.repeat = true;
        hth_input_handle_event(input, &event);
        assert(hth_input_key_down(input, HTH_KEY_W));
        assert(!hth_input_key_pressed(input, HTH_KEY_W));

        event.data.keyboard.key = HTH_KEY_A;
        event.data.keyboard.repeat = false;
        hth_input_handle_event(input, &event);
        assert(hth_input_key_down(input, HTH_KEY_A));
        assert(hth_input_key_pressed(input, HTH_KEY_A));
        event.type = HTH_PLATFORM_EVENT_KEY_UP;
        hth_input_handle_event(input, &event);
        event.data.keyboard.key = HTH_KEY_W;
        hth_input_handle_event(input, &event);
        assert(!hth_input_key_down(input, HTH_KEY_W));
        assert(!hth_input_key_down(input, HTH_KEY_A));
    }
    hth_input_destroy(input);
}

static void test_release_is_idempotent_and_allows_a_new_press(void)
{
    HTHInput *input = hth_input_create();
    HTHPlatformEvent event = {0};

    assert(input != NULL);
    hth_input_begin_frame(input);
    event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
    event.data.keyboard.key = HTH_KEY_W;
    hth_input_handle_event(input, &event);

    event.type = HTH_PLATFORM_EVENT_KEY_UP;
    hth_input_handle_event(input, &event); /* Normalized release. */
    hth_input_handle_event(input, &event); /* Late real SDL release. */
    assert(!hth_input_key_down(input, HTH_KEY_W));
    assert(hth_input_key_released(input, HTH_KEY_W));

    hth_input_begin_frame(input);
    event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
    event.data.keyboard.repeat = false;
    hth_input_handle_event(input, &event);
    assert(hth_input_key_down(input, HTH_KEY_W));
    assert(hth_input_key_pressed(input, HTH_KEY_W));
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
    event.data.motion.delta_x = 1.0;
    event.data.motion.delta_y = 3.0;
    hth_input_handle_event(input, &event);
    event.type = HTH_PLATFORM_EVENT_MOUSE_WHEEL;
    event.data.wheel.x = 1.0;
    event.data.wheel.y = -1.0;
    hth_input_handle_event(input, &event);

    hth_input_mouse_delta(input, &x, &y);
    assert(x == 5.0 && y == 1.0);
    hth_input_clear_mouse_delta(input);
    hth_input_mouse_delta(input, &x, &y);
    assert(x == 0.0 && y == 0.0);
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

static void test_mouse_button_edges(void)
{
    HTHInput *input = hth_input_create();
    HTHPlatformEvent event = {0};

    assert(input != NULL);
    hth_input_begin_frame(input);
    event.type = HTH_PLATFORM_EVENT_MOUSE_BUTTON_DOWN;
    event.data.mouse_button.button = HTH_MOUSE_LEFT;
    hth_input_handle_event(input, &event);
    assert(hth_input_mouse_button_down(input, HTH_MOUSE_LEFT));
    assert(hth_input_mouse_button_pressed(input, HTH_MOUSE_LEFT));

    hth_input_begin_frame(input);
    assert(hth_input_mouse_button_down(input, HTH_MOUSE_LEFT));
    assert(!hth_input_mouse_button_pressed(input, HTH_MOUSE_LEFT));
    event.type = HTH_PLATFORM_EVENT_MOUSE_BUTTON_UP;
    hth_input_handle_event(input, &event);
    assert(!hth_input_mouse_button_down(input, HTH_MOUSE_LEFT));
    assert(hth_input_mouse_button_released(input, HTH_MOUSE_LEFT));
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

static void send_key_event(HTHInput *input, HTHPlatformEventType type,
                           HTHKey key)
{
    HTHPlatformEvent event = {0};

    event.type = type;
    event.data.keyboard.key = key;
    hth_input_handle_event(input, &event);
}

static void interrupt_focus(HTHInput *input)
{
    HTHPlatformEvent event = {0};

    event.type = HTH_PLATFORM_EVENT_FOCUS_LOST;
    hth_input_handle_event(input, &event);
    event.type = HTH_PLATFORM_EVENT_FOCUS_GAINED;
    hth_input_handle_event(input, &event);
}

static void test_movement_keys_recover_on_first_fresh_press(void)
{
    static const HTHKey movement_keys[] = {
        HTH_KEY_W, HTH_KEY_A, HTH_KEY_S, HTH_KEY_D
    };
    HTHInput *input = hth_input_create();
    size_t index;

    assert(input != NULL);
    for (index = 0U; index < sizeof(movement_keys) /
                               sizeof(movement_keys[0]); ++index) {
        HTHKey key = movement_keys[index];

        hth_input_begin_frame(input);
        send_key_event(input, HTH_PLATFORM_EVENT_KEY_DOWN, key);
        assert(hth_input_key_down(input, key));
        interrupt_focus(input);
        assert(!hth_input_key_down(input, key));

        hth_input_begin_frame(input);
        send_key_event(input, HTH_PLATFORM_EVENT_KEY_DOWN, key);
        assert(hth_input_key_down(input, key));
        assert(hth_input_key_pressed(input, key));
        send_key_event(input, HTH_PLATFORM_EVENT_KEY_UP, key);
        assert(!hth_input_key_down(input, key));
        assert(hth_input_key_released(input, key));
    }
    hth_input_destroy(input);
}

static void test_repeated_focus_recovery_key_switch_and_multi_key(void)
{
    HTHInput *input = hth_input_create();

    assert(input != NULL);
    hth_input_begin_frame(input);
    send_key_event(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_W);
    interrupt_focus(input);
    assert(!hth_input_key_down(input, HTH_KEY_W));
    send_key_event(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_A);
    assert(hth_input_key_down(input, HTH_KEY_A));
    send_key_event(input, HTH_PLATFORM_EVENT_KEY_UP, HTH_KEY_A);

    send_key_event(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_W);
    send_key_event(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_D);
    assert(hth_input_key_down(input, HTH_KEY_W));
    assert(hth_input_key_down(input, HTH_KEY_D));
    interrupt_focus(input);
    assert(!hth_input_key_down(input, HTH_KEY_W));
    assert(!hth_input_key_down(input, HTH_KEY_D));
    send_key_event(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_D);
    assert(hth_input_key_down(input, HTH_KEY_D));
    send_key_event(input, HTH_PLATFORM_EVENT_KEY_UP, HTH_KEY_D);

    interrupt_focus(input);
    send_key_event(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_W);
    assert(hth_input_key_down(input, HTH_KEY_W));
    hth_input_destroy(input);
}

static void inject_motion(HTHInput *input, double delta_x, double delta_y)
{
    HTHPlatformEvent event = {0};

    event.type = HTH_PLATFORM_EVENT_MOUSE_MOTION;
    event.data.motion.delta_x = delta_x;
    event.data.motion.delta_y = delta_y;
    hth_input_handle_event(input, &event);
}

static void verify_capture_transition_discard(HTHInput *input)
{
    double x;
    double y;

    hth_input_begin_capture_transition_discard(input);
    assert(hth_input_mouse_motion_discard_active(input));
    inject_motion(input, 100.0, 80.0);
    hth_input_mouse_delta(input, &x, &y);
    assert(x == 0.0 && y == 0.0);

    hth_input_end_frame(input);
    hth_input_begin_frame(input);
    assert(hth_input_mouse_motion_discard_active(input));
    inject_motion(input, -60.0, 40.0);
    hth_input_mouse_delta(input, &x, &y);
    assert(x == 0.0 && y == 0.0);

    hth_input_end_frame(input);
    hth_input_begin_frame(input);
    assert(!hth_input_mouse_motion_discard_active(input));
    inject_motion(input, 3.0, -2.0);
    hth_input_mouse_delta(input, &x, &y);
    assert(x == 3.0 && y == -2.0);
}

static void test_capture_transition_motion_discard(void)
{
    HTHInput *input = hth_input_create();

    assert(input != NULL);
    hth_input_begin_frame(input);
    verify_capture_transition_discard(input); /* Capture enable. */
    hth_input_end_frame(input);
    hth_input_begin_frame(input);
    verify_capture_transition_discard(input); /* Capture release. */
    hth_input_destroy(input);
}

int main(void)
{
    test_keyboard_edges();
    test_repeat_restores_held_state_without_pressed_edge();
    test_reconciled_release_then_repeat_recovers_movement_keys();
    test_repeat_recovery_multi_key_and_rapid_edges();
    test_five_repeat_recovery_cycles_and_cross_key();
    test_release_is_idempotent_and_allows_a_new_press();
    test_mouse_transients();
    test_mouse_button_edges();
    test_focus_loss();
    test_movement_keys_recover_on_first_fresh_press();
    test_repeated_focus_recovery_key_switch_and_multi_key();
    test_capture_transition_motion_discard();
    return 0;
}
