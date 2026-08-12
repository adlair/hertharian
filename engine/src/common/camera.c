#include "hth_camera.h"

#include <stddef.h>

void hth_camera_init_default(HTHCamera *camera)
{
    if (camera == NULL) {
        return;
    }
    camera->position = hth_vec3(0.0F, 0.0F, 3.0F);
    camera->forward = hth_vec3(0.0F, 0.0F, -1.0F);
    camera->up = hth_vec3(0.0F, 1.0F, 0.0F);
    camera->vertical_fov_radians = hth_degrees_to_radians(75.0F);
    camera->near_plane = 0.1F;
    camera->far_plane = 1000.0F;
}

bool hth_camera_view_matrix(const HTHCamera *camera, HTHMat4 *out_view)
{
    return camera != NULL &&
           hth_mat4_look_direction(camera->position, camera->forward,
                                   camera->up, out_view);
}

bool hth_camera_projection_matrix(const HTHCamera *camera,
                                  uint32_t framebuffer_width,
                                  uint32_t framebuffer_height,
                                  HTHMat4 *out_projection)
{
    float aspect;

    if (camera == NULL || framebuffer_width == 0 || framebuffer_height == 0) {
        return false;
    }
    aspect = (float)framebuffer_width / (float)framebuffer_height;
    return hth_mat4_perspective(camera->vertical_fov_radians, aspect,
                                camera->near_plane, camera->far_plane,
                                out_projection);
}
