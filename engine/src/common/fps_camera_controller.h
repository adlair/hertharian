#ifndef HTH_FPS_CAMERA_CONTROLLER_H
#define HTH_FPS_CAMERA_CONTROLLER_H

#include "hth_camera.h"
#include "hth_input.h"

#include <stdbool.h>

typedef struct HTHFPSCameraController HTHFPSCameraController;

typedef enum {
    HTH_FPS_CAPTURE_NONE = 0,
    HTH_FPS_CAPTURE_ENABLE,
    HTH_FPS_CAPTURE_DISABLE
} HTHFPSCaptureAction;

HTHFPSCameraController *hth_fps_camera_controller_create(
    const HTHCamera *camera);
void hth_fps_camera_controller_destroy(HTHFPSCameraController *controller);
HTHFPSCaptureAction hth_fps_camera_controller_capture_action(
    const HTHFPSCameraController *controller, const HTHInput *input);
void hth_fps_camera_controller_set_capture(
    HTHFPSCameraController *controller, bool active);
bool hth_fps_camera_controller_capture_active(
    const HTHFPSCameraController *controller);
void hth_fps_camera_controller_update(HTHFPSCameraController *controller,
                                      HTHCamera *camera,
                                      const HTHInput *input,
                                      double delta_seconds,
                                      bool debug_input);

#endif
