#include "hth_math.h"

#include <math.h>
#include <stddef.h>

static const float hth_pi = 3.14159265358979323846F;
static const float hth_normalize_epsilon = 1.0e-6F;

float hth_degrees_to_radians(float degrees)
{
    return degrees * (hth_pi / 180.0F);
}

HTHVec2 hth_vec2(float x, float y)
{
    HTHVec2 result = {x, y};
    return result;
}

HTHVec3 hth_vec3(float x, float y, float z)
{
    HTHVec3 result = {x, y, z};
    return result;
}

HTHVec3 hth_vec3_add(HTHVec3 left, HTHVec3 right)
{
    return hth_vec3(left.x + right.x, left.y + right.y, left.z + right.z);
}

HTHVec3 hth_vec3_subtract(HTHVec3 left, HTHVec3 right)
{
    return hth_vec3(left.x - right.x, left.y - right.y, left.z - right.z);
}

HTHVec3 hth_vec3_scale(HTHVec3 vector, float scalar)
{
    return hth_vec3(vector.x * scalar, vector.y * scalar, vector.z * scalar);
}

float hth_vec3_dot(HTHVec3 left, HTHVec3 right)
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

HTHVec3 hth_vec3_cross(HTHVec3 left, HTHVec3 right)
{
    return hth_vec3(left.y * right.z - left.z * right.y,
                    left.z * right.x - left.x * right.z,
                    left.x * right.y - left.y * right.x);
}

float hth_vec3_length(HTHVec3 vector)
{
    return sqrtf(hth_vec3_dot(vector, vector));
}

bool hth_vec3_normalize(HTHVec3 vector, HTHVec3 *out_normalized)
{
    float length;

    if (out_normalized == NULL) {
        return false;
    }
    *out_normalized = hth_vec3(0.0F, 0.0F, 0.0F);
    length = hth_vec3_length(vector);
    if (!isfinite(length) || length <= hth_normalize_epsilon) {
        return false;
    }
    *out_normalized = hth_vec3_scale(vector, 1.0F / length);
    return true;
}

HTHMat4 hth_mat4_identity(void)
{
    HTHMat4 result = {{0.0F}};
    result.elements[0] = 1.0F;
    result.elements[5] = 1.0F;
    result.elements[10] = 1.0F;
    result.elements[15] = 1.0F;
    return result;
}

HTHMat4 hth_mat4_multiply(HTHMat4 left, HTHMat4 right)
{
    HTHMat4 result = {{0.0F}};
    size_t column;
    size_t row;
    size_t term;

    for (column = 0; column < 4; ++column) {
        for (row = 0; row < 4; ++row) {
            for (term = 0; term < 4; ++term) {
                result.elements[column * 4 + row] +=
                    left.elements[term * 4 + row] *
                    right.elements[column * 4 + term];
            }
        }
    }
    return result;
}

HTHMat4 hth_mat4_translation(HTHVec3 translation)
{
    HTHMat4 result = hth_mat4_identity();
    result.elements[12] = translation.x;
    result.elements[13] = translation.y;
    result.elements[14] = translation.z;
    return result;
}

bool hth_mat4_perspective(float vertical_fov_radians, float aspect,
                          float near_plane, float far_plane,
                          HTHMat4 *out_projection)
{
    float focal_length;
    float half_fov;
    HTHMat4 result = {{0.0F}};

    if (out_projection == NULL || !isfinite(vertical_fov_radians) ||
        !isfinite(aspect) || !isfinite(near_plane) || !isfinite(far_plane) ||
        vertical_fov_radians <= 0.0F || vertical_fov_radians >= hth_pi ||
        aspect <= 0.0F || near_plane <= 0.0F || far_plane <= near_plane) {
        return false;
    }

    half_fov = vertical_fov_radians * 0.5F;
    focal_length = 1.0F / tanf(half_fov);
    if (!isfinite(focal_length)) {
        return false;
    }

    result.elements[0] = focal_length / aspect;
    result.elements[5] = focal_length;
    result.elements[10] = (far_plane + near_plane) /
                          (near_plane - far_plane);
    result.elements[11] = -1.0F;
    result.elements[14] = (2.0F * far_plane * near_plane) /
                          (near_plane - far_plane);
    *out_projection = result;
    return true;
}

bool hth_mat4_look_direction(HTHVec3 position, HTHVec3 forward, HTHVec3 up,
                             HTHMat4 *out_view)
{
    HTHVec3 normalized_forward;
    HTHVec3 right;
    HTHVec3 corrected_up;
    HTHMat4 result = hth_mat4_identity();

    if (out_view == NULL || !isfinite(position.x) ||
        !isfinite(position.y) || !isfinite(position.z) ||
        !hth_vec3_normalize(forward, &normalized_forward) ||
        !hth_vec3_normalize(hth_vec3_cross(normalized_forward, up), &right)) {
        return false;
    }
    corrected_up = hth_vec3_cross(right, normalized_forward);

    result.elements[0] = right.x;
    result.elements[4] = right.y;
    result.elements[8] = right.z;
    result.elements[12] = -hth_vec3_dot(right, position);
    result.elements[1] = corrected_up.x;
    result.elements[5] = corrected_up.y;
    result.elements[9] = corrected_up.z;
    result.elements[13] = -hth_vec3_dot(corrected_up, position);
    result.elements[2] = -normalized_forward.x;
    result.elements[6] = -normalized_forward.y;
    result.elements[10] = -normalized_forward.z;
    result.elements[14] = hth_vec3_dot(normalized_forward, position);
    *out_view = result;
    return true;
}

HTHVec4 hth_mat4_transform_vec4(HTHMat4 matrix, HTHVec4 vector)
{
    HTHVec4 result;
    result.x = matrix.elements[0] * vector.x +
               matrix.elements[4] * vector.y +
               matrix.elements[8] * vector.z +
               matrix.elements[12] * vector.w;
    result.y = matrix.elements[1] * vector.x +
               matrix.elements[5] * vector.y +
               matrix.elements[9] * vector.z +
               matrix.elements[13] * vector.w;
    result.z = matrix.elements[2] * vector.x +
               matrix.elements[6] * vector.y +
               matrix.elements[10] * vector.z +
               matrix.elements[14] * vector.w;
    result.w = matrix.elements[3] * vector.x +
               matrix.elements[7] * vector.y +
               matrix.elements[11] * vector.z +
               matrix.elements[15] * vector.w;
    return result;
}
