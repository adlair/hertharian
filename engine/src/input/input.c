#include "input_internal.h"

#include <stdlib.h>
#include <string.h>

struct HTHInput {
    bool keys_down[HTH_KEY_COUNT];
    bool keys_pressed[HTH_KEY_COUNT];
    bool keys_released[HTH_KEY_COUNT];
    bool mouse_down[HTH_MOUSE_BUTTON_COUNT];
    bool mouse_pressed[HTH_MOUSE_BUTTON_COUNT];
    bool mouse_released[HTH_MOUSE_BUTTON_COUNT];
    double mouse_x;
    double mouse_y;
    double mouse_delta_x;
    double mouse_delta_y;
    double wheel_x;
    double wheel_y;
    unsigned int relative_motion_discard_end_frames;
};

static const unsigned int capture_transition_end_frames = 2;

static bool valid_key(HTHKey key)
{
    return key > HTH_KEY_UNKNOWN && key < HTH_KEY_COUNT;
}

static bool valid_button(HTHMouseButton button)
{
    return button > HTH_MOUSE_UNKNOWN && button < HTH_MOUSE_BUTTON_COUNT;
}

HTHInput *hth_input_create(void)
{
    return calloc(1, sizeof(HTHInput));
}

void hth_input_destroy(HTHInput *input)
{
    free(input);
}

void hth_input_begin_frame(HTHInput *input)
{
    if (input == NULL) {
        return;
    }

    memset(input->keys_pressed, 0, sizeof(input->keys_pressed));
    memset(input->keys_released, 0, sizeof(input->keys_released));
    memset(input->mouse_pressed, 0, sizeof(input->mouse_pressed));
    memset(input->mouse_released, 0, sizeof(input->mouse_released));
    input->mouse_delta_x = 0.0;
    input->mouse_delta_y = 0.0;
    input->wheel_x = 0.0;
    input->wheel_y = 0.0;
}

void hth_input_end_frame(HTHInput *input)
{
    if (input != NULL && input->relative_motion_discard_end_frames > 0) {
        input->relative_motion_discard_end_frames--;
    }
}

void hth_input_clear_mouse_delta(HTHInput *input)
{
    if (input != NULL) {
        input->mouse_delta_x = 0.0;
        input->mouse_delta_y = 0.0;
    }
}

void hth_input_begin_capture_transition_discard(HTHInput *input)
{
    if (input != NULL) {
        /* Remainder of this frame plus the next complete frame. */
        input->relative_motion_discard_end_frames =
            capture_transition_end_frames;
        hth_input_clear_mouse_delta(input);
    }
}

bool hth_input_mouse_motion_discard_active(const HTHInput *input)
{
    return input != NULL && input->relative_motion_discard_end_frames > 0;
}

static void clear_for_focus_loss(HTHInput *input)
{
    size_t index;

    for (index = 0; index < HTH_KEY_COUNT; index++) {
        if (input->keys_down[index]) {
            input->keys_released[index] = true;
        }
        input->keys_down[index] = false;
    }

    for (index = 0; index < HTH_MOUSE_BUTTON_COUNT; index++) {
        if (input->mouse_down[index]) {
            input->mouse_released[index] = true;
        }
        input->mouse_down[index] = false;
    }

    hth_input_clear_mouse_delta(input);
}

void hth_input_handle_event(HTHInput *input, const HTHPlatformEvent *event)
{
    HTHKey key;
    HTHMouseButton button;

    if (input == NULL || event == NULL) {
        return;
    }

    switch (event->type) {
    case HTH_PLATFORM_EVENT_KEY_DOWN:
        key = event->data.keyboard.key;
        if (valid_key(key) && !input->keys_down[key]) {
            input->keys_down[key] = true;
            input->keys_pressed[key] = true;
        }
        break;
    case HTH_PLATFORM_EVENT_KEY_UP:
        key = event->data.keyboard.key;
        if (valid_key(key) && input->keys_down[key]) {
            input->keys_down[key] = false;
            input->keys_released[key] = true;
        }
        break;
    case HTH_PLATFORM_EVENT_MOUSE_MOTION:
        input->mouse_x = event->data.motion.x;
        input->mouse_y = event->data.motion.y;
        if (hth_input_mouse_motion_discard_active(input)) {
            break;
        }
        input->mouse_delta_x += event->data.motion.delta_x;
        input->mouse_delta_y += event->data.motion.delta_y;
        break;
    case HTH_PLATFORM_EVENT_MOUSE_BUTTON_DOWN:
        button = event->data.mouse_button.button;
        input->mouse_x = event->data.mouse_button.x;
        input->mouse_y = event->data.mouse_button.y;
        if (valid_button(button) && !input->mouse_down[button]) {
            input->mouse_down[button] = true;
            input->mouse_pressed[button] = true;
        }
        break;
    case HTH_PLATFORM_EVENT_MOUSE_BUTTON_UP:
        button = event->data.mouse_button.button;
        input->mouse_x = event->data.mouse_button.x;
        input->mouse_y = event->data.mouse_button.y;
        if (valid_button(button) && input->mouse_down[button]) {
            input->mouse_down[button] = false;
            input->mouse_released[button] = true;
        }
        break;
    case HTH_PLATFORM_EVENT_MOUSE_WHEEL:
        input->wheel_x += event->data.wheel.x;
        input->wheel_y += event->data.wheel.y;
        break;
    case HTH_PLATFORM_EVENT_FOCUS_LOST:
        clear_for_focus_loss(input);
        break;
    default:
        break;
    }
}

bool hth_input_key_down(const HTHInput *input, HTHKey key)
{
    return input != NULL && valid_key(key) && input->keys_down[key];
}

bool hth_input_key_pressed(const HTHInput *input, HTHKey key)
{
    return input != NULL && valid_key(key) && input->keys_pressed[key];
}

bool hth_input_key_released(const HTHInput *input, HTHKey key)
{
    return input != NULL && valid_key(key) && input->keys_released[key];
}

bool hth_input_mouse_button_down(const HTHInput *input, HTHMouseButton button)
{
    return input != NULL && valid_button(button) && input->mouse_down[button];
}

bool hth_input_mouse_button_pressed(const HTHInput *input, HTHMouseButton button)
{
    return input != NULL && valid_button(button) && input->mouse_pressed[button];
}

bool hth_input_mouse_button_released(const HTHInput *input, HTHMouseButton button)
{
    return input != NULL && valid_button(button) && input->mouse_released[button];
}

void hth_input_mouse_position(const HTHInput *input, double *x, double *y)
{
    if (x != NULL) { *x = input != NULL ? input->mouse_x : 0.0; }
    if (y != NULL) { *y = input != NULL ? input->mouse_y : 0.0; }
}

void hth_input_mouse_delta(const HTHInput *input, double *x, double *y)
{
    if (x != NULL) { *x = input != NULL ? input->mouse_delta_x : 0.0; }
    if (y != NULL) { *y = input != NULL ? input->mouse_delta_y : 0.0; }
}

void hth_input_mouse_wheel(const HTHInput *input, double *x, double *y)
{
    if (x != NULL) { *x = input != NULL ? input->wheel_x : 0.0; }
    if (y != NULL) { *y = input != NULL ? input->wheel_y : 0.0; }
}
