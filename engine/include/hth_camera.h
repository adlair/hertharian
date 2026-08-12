#ifndef HTH_CAMERA_H
#define HTH_CAMERA_H

#include "hth_math.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    HTHVec3 position;
    HTHVec3 forward;
    HTHVec3 up;
    float vertical_fov_radians;
    float near_plane;
    float far_plane;
} HTHCamera;

void hth_camera_init_default(HTHCamera *camera);
bool hth_camera_view_matrix(const HTHCamera *camera, HTHMat4 *out_view);
bool hth_camera_projection_matrix(const HTHCamera *camera,
                                  uint32_t framebuffer_width,
                                  uint32_t framebuffer_height,
                                  HTHMat4 *out_projection);

#endif
