#include "fps_camera_controller.h"

#include "hth_math.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct HTHFPSCameraController {
    float yaw_radians;
    float pitch_radians;
    float mouse_sensitivity_radians;
    float pitch_limit_radians;
    bool capture_active;
};

static float clamp_float(float value, float minimum, float maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static HTHVec3 forward_from_angles(float yaw, float pitch)
{
    float pitch_cosine = cosf(pitch);

    return hth_vec3(sinf(yaw) * pitch_cosine,
                    sinf(pitch),
                    -cosf(yaw) * pitch_cosine);
}

HTHFPSCameraController *hth_fps_camera_controller_create(
    const HTHCamera *camera)
{
    HTHFPSCameraController *controller;
    HTHVec3 forward;

    if (camera == NULL || !hth_vec3_normalize(camera->forward, &forward)) {
        return NULL;
    }

    controller = calloc(1, sizeof(*controller));
    if (controller == NULL) {
        return NULL;
    }
    controller->mouse_sensitivity_radians =
        hth_degrees_to_radians(0.1F);
    controller->pitch_limit_radians = hth_degrees_to_radians(89.0F);
    controller->yaw_radians = atan2f(forward.x, -forward.z);
    controller->pitch_radians = clamp_float(
        asinf(clamp_float(forward.y, -1.0F, 1.0F)),
        -controller->pitch_limit_radians,
        controller->pitch_limit_radians);
    return controller;
}

void hth_fps_camera_controller_destroy(HTHFPSCameraController *controller)
{
    free(controller);
}

HTHFPSCaptureAction hth_fps_camera_controller_capture_action(
    const HTHFPSCameraController *controller, const HTHInput *input)
{
    if (controller == NULL || input == NULL) {
        return HTH_FPS_CAPTURE_NONE;
    }
    if (controller->capture_active) {
        return hth_input_key_pressed(input, HTH_KEY_ESCAPE)
            ? HTH_FPS_CAPTURE_DISABLE
            : HTH_FPS_CAPTURE_NONE;
    }
    return hth_input_mouse_button_pressed(input, HTH_MOUSE_LEFT)
        ? HTH_FPS_CAPTURE_ENABLE
        : HTH_FPS_CAPTURE_NONE;
}

void hth_fps_camera_controller_set_capture(
    HTHFPSCameraController *controller, bool active)
{
    if (controller != NULL) {
        controller->capture_active = active;
    }
}

bool hth_fps_camera_controller_capture_active(
    const HTHFPSCameraController *controller)
{
    return controller != NULL && controller->capture_active;
}

void hth_fps_camera_controller_update(HTHFPSCameraController *controller,
                                      HTHCamera *camera,
                                      const HTHInput *input,
                                      bool debug_input)
{
    double mouse_x;
    double mouse_y;
    float mouse_delta_x;
    float mouse_delta_y;
    float pitch_before;
    float yaw_before;

    if (controller == NULL || camera == NULL || input == NULL) {
        return;
    }

    if (controller->capture_active) {
        hth_input_mouse_delta(input, &mouse_x, &mouse_y);
        mouse_delta_x = (float)mouse_x;
        mouse_delta_y = (float)mouse_y;
        yaw_before = controller->yaw_radians;
        pitch_before = controller->pitch_radians;
        if (isfinite(mouse_delta_x) && isfinite(mouse_delta_y)) {
            controller->yaw_radians +=
                mouse_delta_x * controller->mouse_sensitivity_radians;
            controller->yaw_radians = remainderf(
                controller->yaw_radians, hth_degrees_to_radians(360.0F));
            controller->pitch_radians = clamp_float(
                controller->pitch_radians -
                    mouse_delta_y * controller->mouse_sensitivity_radians,
                -controller->pitch_limit_radians,
                controller->pitch_limit_radians);
        }
        if (debug_input && (mouse_delta_x != 0.0F || mouse_delta_y != 0.0F)) {
            printf("FPS input: controller yaw %.9g -> %.9g, "
                   "pitch %.9g -> %.9g radians\n",
                   (double)yaw_before, (double)controller->yaw_radians,
                   (double)pitch_before, (double)controller->pitch_radians);
        }
    }

    camera->forward = forward_from_angles(controller->yaw_radians,
                                          controller->pitch_radians);
}
