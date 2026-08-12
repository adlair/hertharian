#include "hth_camera.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

static const float epsilon = 1.0e-5F;

static bool close_enough(float left, float right)
{
    return fabsf(left - right) <= epsilon;
}

int main(void)
{
    HTHCamera camera;
    HTHMat4 projection;
    HTHMat4 view;
    HTHVec4 camera_position;
    HTHVec4 world_origin;

    hth_camera_init_default(&camera);
    if (!hth_camera_view_matrix(&camera, &view)) {
        fputs("Default camera view failed.\n", stderr);
        return 1;
    }
    camera_position = hth_mat4_transform_vec4(
        view, (HTHVec4){0.0F, 0.0F, 3.0F, 1.0F});
    world_origin = hth_mat4_transform_vec4(
        view, (HTHVec4){0.0F, 0.0F, 0.0F, 1.0F});
    if (!close_enough(camera_position.x, 0.0F) ||
        !close_enough(camera_position.y, 0.0F) ||
        !close_enough(camera_position.z, 0.0F) ||
        !close_enough(world_origin.z, -3.0F) ||
        !hth_camera_projection_matrix(&camera, 1280, 720, &projection) ||
        !isfinite(projection.elements[0]) ||
        hth_camera_projection_matrix(&camera, 0, 720, &projection) ||
        hth_camera_projection_matrix(&camera, 1280, 0, &projection)) {
        fputs("Default camera projection failed.\n", stderr);
        return 1;
    }

    camera.near_plane = 10.0F;
    camera.far_plane = 1.0F;
    if (hth_camera_projection_matrix(&camera, 1280, 720, &projection)) {
        fputs("Invalid camera planes were accepted.\n", stderr);
        return 1;
    }
    hth_camera_init_default(&camera);
    camera.vertical_fov_radians = 0.0F;
    if (hth_camera_projection_matrix(&camera, 1280, 720, &projection)) {
        fputs("Invalid camera FOV was accepted.\n", stderr);
        return 1;
    }
    hth_camera_init_default(&camera);
    camera.forward = hth_vec3(0.0F, 0.0F, 0.0F);
    if (hth_camera_view_matrix(&camera, &view)) {
        fputs("Zero camera direction was accepted.\n", stderr);
        return 1;
    }
    camera.forward = hth_vec3(0.0F, 1.0F, 0.0F);
    if (hth_camera_view_matrix(&camera, &view)) {
        fputs("Parallel camera forward/up were accepted.\n", stderr);
        return 1;
    }

    puts("Camera foundation tests passed.");
    return 0;
}
