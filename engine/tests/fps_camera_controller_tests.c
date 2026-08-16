#include "event.h"
#include "fps_camera_controller.h"
#include "hth_camera.h"
#include "hth_input.h"
#include "hth_math.h"
#include "input_internal.h"
#include "mouse_source.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>

static const float epsilon = 1.0e-4F;

static bool close_enough(float left, float right)
{
    return fabsf(left - right) <= epsilon;
}

static void inject_key(HTHInput *input, HTHKey key, bool down)
{
    HTHPlatformEvent event = {0};
    event.type = down ? HTH_PLATFORM_EVENT_KEY_DOWN
                      : HTH_PLATFORM_EVENT_KEY_UP;
    event.data.keyboard.key = key;
    hth_input_handle_event(input, &event);
}

static void inject_mouse_delta(HTHInput *input, double x, double y)
{
    HTHPlatformEvent event = {0};
    event.type = HTH_PLATFORM_EVENT_MOUSE_MOTION;
    event.data.motion.delta_x = x;
    event.data.motion.delta_y = y;
    hth_input_handle_event(input, &event);
}

static bool inject_filtered_mouse_delta(HTHRelativeMouseFilter *filter,
                                        HTHInput *input,
                                        uint32_t source_id,
                                        double x,
                                        double y)
{
    double corrected_x;
    double corrected_y;

    if (hth_relative_mouse_filter_motion(
            filter, true, true, 7U, source_id, x, y,
            &corrected_x, &corrected_y) !=
        HTH_RELATIVE_MOUSE_MOTION_ACCEPT) {
        return false;
    }
    inject_mouse_delta(input, corrected_x, corrected_y);
    return true;
}

static void inject_mouse_button(HTHInput *input, HTHMouseButton button,
                                bool down)
{
    HTHPlatformEvent event = {0};
    event.type = down ? HTH_PLATFORM_EVENT_MOUSE_BUTTON_DOWN
                      : HTH_PLATFORM_EVENT_MOUSE_BUTTON_UP;
    event.data.mouse_button.button = button;
    hth_input_handle_event(input, &event);
}

static HTHFPSCameraController *create_controller(HTHCamera *camera,
                                                 HTHInput **input)
{
    HTHFPSCameraController *controller;

    hth_camera_init_default(camera);
    *input = hth_input_create();
    assert(*input != NULL);
    controller = hth_fps_camera_controller_create(camera);
    assert(controller != NULL);
    hth_input_begin_frame(*input);
    return controller;
}

static void destroy_controller(HTHFPSCameraController *controller,
                               HTHInput *input)
{
    hth_fps_camera_controller_destroy(controller);
    hth_input_destroy(input);
}

static void test_orientation(void)
{
    HTHCamera camera;
    HTHFPSCameraController *controller;
    HTHInput *input;
    HTHVec3 right;

    controller = create_controller(&camera, &input);
    inject_mouse_delta(input, 300.0, -300.0);
    hth_fps_camera_controller_update(controller, &camera, input, false);
    assert(close_enough(camera.forward.x, 0.0F));
    assert(close_enough(camera.forward.y, 0.0F));
    assert(close_enough(camera.forward.z, -1.0F));
    assert(hth_vec3_normalize(hth_vec3_cross(camera.forward, camera.up),
                              &right));
    assert(close_enough(right.x, 1.0F));
    assert(close_enough(right.y, 0.0F));
    assert(close_enough(right.z, 0.0F));

    hth_fps_camera_controller_set_capture(controller, true);
    hth_input_clear_mouse_delta(input);
    inject_mouse_delta(input, 900.0, 0.0);
    hth_fps_camera_controller_update(controller, &camera, input, false);
    assert(close_enough(camera.forward.x, 1.0F));
    assert(close_enough(camera.forward.y, 0.0F));
    assert(close_enough(camera.forward.z, 0.0F));
    destroy_controller(controller, input);

    controller = create_controller(&camera, &input);
    hth_fps_camera_controller_set_capture(controller, true);
    inject_mouse_delta(input, 0.0, -100.0);
    hth_fps_camera_controller_update(controller, &camera, input, false);
    assert(camera.forward.y > 0.0F);
    destroy_controller(controller, input);
}

static void test_pitch_clamp_and_mouse_delta_semantics(void)
{
    HTHCamera camera_fast;
    HTHCamera camera_slow;
    HTHFPSCameraController *fast;
    HTHFPSCameraController *slow;
    HTHInput *fast_input;
    HTHInput *slow_input;
    float pitch_limit_sine = sinf(hth_degrees_to_radians(89.0F));

    slow = create_controller(&camera_slow, &slow_input);
    fast = create_controller(&camera_fast, &fast_input);
    hth_fps_camera_controller_set_capture(slow, true);
    hth_fps_camera_controller_set_capture(fast, true);
    inject_mouse_delta(slow_input, 100.0, -2000.0);
    inject_mouse_delta(fast_input, 100.0, -2000.0);
    hth_fps_camera_controller_update(slow, &camera_slow, slow_input, false);
    hth_fps_camera_controller_update(fast, &camera_fast, fast_input, false);
    assert(close_enough(camera_slow.forward.y, pitch_limit_sine));
    assert(close_enough(camera_fast.forward.y, pitch_limit_sine));
    assert(close_enough(camera_slow.forward.x, camera_fast.forward.x));
    assert(close_enough(camera_slow.forward.z, camera_fast.forward.z));
    destroy_controller(slow, slow_input);
    destroy_controller(fast, fast_input);

    slow = create_controller(&camera_slow, &slow_input);
    hth_fps_camera_controller_set_capture(slow, true);
    inject_mouse_delta(slow_input, 0.0, 2000.0);
    hth_fps_camera_controller_update(slow, &camera_slow, slow_input, false);
    assert(close_enough(camera_slow.forward.y, -pitch_limit_sine));
    destroy_controller(slow, slow_input);
}

static void test_controller_does_not_translate_camera(void)
{
    HTHCamera camera;
    HTHFPSCameraController *controller;
    HTHInput *input;
    HTHVec3 start;

    controller = create_controller(&camera, &input);
    start = camera.position;
    inject_key(input, HTH_KEY_W, true);
    hth_fps_camera_controller_update(controller, &camera, input, false);
    assert(close_enough(camera.position.x, start.x));
    assert(close_enough(camera.position.y, start.y));
    assert(close_enough(camera.position.z, start.z));
    destroy_controller(controller, input);
}

static void test_capture_transitions(void)
{
    HTHCamera camera;
    HTHFPSCameraController *controller;
    HTHInput *input;

    controller = create_controller(&camera, &input);
    inject_mouse_button(input, HTH_MOUSE_LEFT, true);
    assert(hth_fps_camera_controller_capture_action(controller, input) ==
           HTH_FPS_CAPTURE_ENABLE);
    hth_fps_camera_controller_set_capture(controller, true);
    assert(hth_fps_camera_controller_capture_active(controller));

    hth_input_begin_frame(input);
    inject_key(input, HTH_KEY_ESCAPE, true);
    assert(hth_fps_camera_controller_capture_action(controller, input) ==
           HTH_FPS_CAPTURE_DISABLE);
    hth_fps_camera_controller_set_capture(controller, false);
    assert(!hth_fps_camera_controller_capture_active(controller));
    assert(hth_fps_camera_controller_capture_action(controller, input) ==
           HTH_FPS_CAPTURE_NONE);

    hth_input_begin_frame(input);
    inject_key(input, HTH_KEY_ESCAPE, false);
    inject_mouse_button(input, HTH_MOUSE_LEFT, false);
    hth_input_begin_frame(input);
    inject_mouse_button(input, HTH_MOUSE_LEFT, true);
    assert(hth_fps_camera_controller_capture_action(controller, input) ==
           HTH_FPS_CAPTURE_ENABLE);
    destroy_controller(controller, input);
}

static void test_capture_transition_does_not_reach_controller(void)
{
    HTHCamera camera;
    HTHFPSCameraController *controller;
    HTHInput *input;

    controller = create_controller(&camera, &input);
    hth_fps_camera_controller_set_capture(controller, true);
    hth_input_begin_capture_transition_discard(input);
    inject_mouse_delta(input, 100.0, 80.0);
    hth_fps_camera_controller_update(controller, &camera, input, false);
    assert(close_enough(camera.forward.x, 0.0F));
    assert(close_enough(camera.forward.y, 0.0F));
    assert(close_enough(camera.forward.z, -1.0F));

    hth_input_end_frame(input);
    hth_input_begin_frame(input);
    inject_mouse_delta(input, -60.0, 40.0);
    hth_fps_camera_controller_update(controller, &camera, input, false);
    assert(close_enough(camera.forward.x, 0.0F));
    assert(close_enough(camera.forward.y, 0.0F));
    assert(close_enough(camera.forward.z, -1.0F));

    hth_input_end_frame(input);
    hth_input_begin_frame(input);
    inject_mouse_delta(input, 10.0, 0.0);
    hth_fps_camera_controller_update(controller, &camera, input, false);
    assert(camera.forward.x > 0.0F);
    destroy_controller(controller, input);
}

static void test_reentry_compensation_does_not_reach_controller(void)
{
    HTHCamera camera;
    HTHFPSCameraController *controller;
    HTHInput *input;
    HTHRelativeMouseFilter filter = {0};
    float expected_yaw = hth_degrees_to_radians(0.2F);
    float expected_pitch = hth_degrees_to_radians(0.3F);

    controller = create_controller(&camera, &input);
    hth_fps_camera_controller_set_capture(controller, true);
    hth_relative_mouse_filter_reset(&filter); /* Capture/focus transition. */

    assert(!inject_filtered_mouse_delta(&filter, input, 6U, 935.0, 659.0));
    assert(!inject_filtered_mouse_delta(&filter, input, 7U, -935.0, -659.0));
    hth_fps_camera_controller_update(controller, &camera, input, false);
    assert(close_enough(camera.forward.x, 0.0F));
    assert(close_enough(camera.forward.y, 0.0F));
    assert(close_enough(camera.forward.z, -1.0F));

    assert(inject_filtered_mouse_delta(&filter, input, 7U, 2.0, -3.0));
    hth_fps_camera_controller_update(controller, &camera, input, false);
    assert(close_enough(camera.forward.x,
                        sinf(expected_yaw) * cosf(expected_pitch)));
    assert(close_enough(camera.forward.y, sinf(expected_pitch)));
    assert(close_enough(camera.forward.z,
                        -cosf(expected_yaw) * cosf(expected_pitch)));

    hth_input_begin_frame(input);
    assert(inject_filtered_mouse_delta(&filter, input, 7U, 1.0, 1.0));
    hth_fps_camera_controller_update(controller, &camera, input, false);
    expected_yaw = hth_degrees_to_radians(0.5F);
    expected_pitch = hth_degrees_to_radians(0.5F);
    assert(close_enough(camera.forward.x,
                        sinf(expected_yaw) * cosf(expected_pitch)));
    assert(close_enough(camera.forward.y, sinf(expected_pitch)));
    assert(close_enough(camera.forward.z,
                        -cosf(expected_yaw) * cosf(expected_pitch)));
    destroy_controller(controller, input);
}

static void test_continuous_filtered_motion_is_directional(void)
{
    static const double horizontal[] = {4.0, -2.0, -1.0, 2.0};
    static const double vertical[] = {-4.0, 2.0, 1.0, -2.0};
    HTHCamera camera;
    HTHFPSCameraController *controller;
    HTHInput *input;
    HTHRelativeMouseFilter filter = {0};
    float previous;
    size_t index;

    controller = create_controller(&camera, &input);
    hth_fps_camera_controller_set_capture(controller, true);
    previous = camera.forward.x;
    for (index = 0; index < sizeof(horizontal) / sizeof(horizontal[0]);
         ++index) {
        assert(inject_filtered_mouse_delta(
            &filter, input, 7U, horizontal[index], 0.0));
        hth_fps_camera_controller_update(controller, &camera, input, false);
        assert(camera.forward.x > previous); /* Sustained left. */
        previous = camera.forward.x;
        hth_input_begin_frame(input);
    }
    destroy_controller(controller, input);

    hth_relative_mouse_filter_reset(&filter);
    controller = create_controller(&camera, &input);
    hth_fps_camera_controller_set_capture(controller, true);
    previous = camera.forward.x;
    for (index = 0; index < sizeof(horizontal) / sizeof(horizontal[0]);
         ++index) {
        assert(inject_filtered_mouse_delta(
            &filter, input, 7U, -horizontal[index], 0.0));
        hth_fps_camera_controller_update(controller, &camera, input, false);
        assert(camera.forward.x < previous); /* Sustained right. */
        previous = camera.forward.x;
        hth_input_begin_frame(input);
    }
    destroy_controller(controller, input);

    hth_relative_mouse_filter_reset(&filter);
    controller = create_controller(&camera, &input);
    hth_fps_camera_controller_set_capture(controller, true);
    previous = camera.forward.y;
    for (index = 0; index < sizeof(vertical) / sizeof(vertical[0]);
         ++index) {
        assert(inject_filtered_mouse_delta(
            &filter, input, 7U, 0.0, vertical[index]));
        hth_fps_camera_controller_update(controller, &camera, input, false);
        assert(camera.forward.y > previous); /* Sustained up. */
        previous = camera.forward.y;
        hth_input_begin_frame(input);
    }
    destroy_controller(controller, input);

    hth_relative_mouse_filter_reset(&filter);
    controller = create_controller(&camera, &input);
    hth_fps_camera_controller_set_capture(controller, true);
    previous = camera.forward.y;
    for (index = 0; index < sizeof(vertical) / sizeof(vertical[0]);
         ++index) {
        assert(inject_filtered_mouse_delta(
            &filter, input, 7U, 0.0, -vertical[index]));
        hth_fps_camera_controller_update(controller, &camera, input, false);
        assert(camera.forward.y < previous); /* Sustained down. */
        previous = camera.forward.y;
        hth_input_begin_frame(input);
    }
    destroy_controller(controller, input);
}

static void test_filtered_reversal_and_diagonal(void)
{
    HTHCamera camera;
    HTHFPSCameraController *controller;
    HTHInput *input;
    HTHRelativeMouseFilter filter = {0};
    float after_left;

    controller = create_controller(&camera, &input);
    hth_fps_camera_controller_set_capture(controller, true);
    assert(inject_filtered_mouse_delta(&filter, input, 7U, 3.0, 0.0));
    hth_fps_camera_controller_update(controller, &camera, input, false);
    after_left = camera.forward.x;
    hth_input_begin_frame(input);
    assert(inject_filtered_mouse_delta(&filter, input, 7U, -5.0, 0.0));
    hth_fps_camera_controller_update(controller, &camera, input, false);
    assert(camera.forward.x < after_left); /* Immediate real reversal. */
    destroy_controller(controller, input);

    hth_relative_mouse_filter_reset(&filter);
    controller = create_controller(&camera, &input);
    hth_fps_camera_controller_set_capture(controller, true);
    assert(inject_filtered_mouse_delta(&filter, input, 7U, 2.0, -3.0));
    hth_fps_camera_controller_update(controller, &camera, input, false);
    assert(camera.forward.x > 0.0F);
    assert(camera.forward.y > 0.0F);
    destroy_controller(controller, input);
}

int main(void)
{
    test_orientation();
    test_pitch_clamp_and_mouse_delta_semantics();
    test_controller_does_not_translate_camera();
    test_capture_transitions();
    test_capture_transition_does_not_reach_controller();
    test_reentry_compensation_does_not_reach_controller();
    test_continuous_filtered_motion_is_directional();
    test_filtered_reversal_and_diagonal();
    return 0;
}
