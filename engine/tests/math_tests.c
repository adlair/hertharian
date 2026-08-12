#include "hth_math.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

static const float epsilon = 1.0e-5F;

static bool close_enough(float left, float right)
{
    return fabsf(left - right) <= epsilon;
}

static bool vec3_matches(HTHVec3 vector, float x, float y, float z)
{
    return close_enough(vector.x, x) && close_enough(vector.y, y) &&
           close_enough(vector.z, z);
}

static bool vec4_matches(HTHVec4 vector, float x, float y, float z, float w)
{
    return close_enough(vector.x, x) && close_enough(vector.y, y) &&
           close_enough(vector.z, z) && close_enough(vector.w, w);
}

static bool test_vectors(void)
{
    HTHVec3 normalized;
    HTHVec3 zero_normalized = hth_vec3(1.0F, 1.0F, 1.0F);
    HTHVec3 x = hth_vec3(1.0F, 0.0F, 0.0F);
    HTHVec3 y = hth_vec3(0.0F, 1.0F, 0.0F);
    HTHVec3 value = hth_vec3(3.0F, 4.0F, 0.0F);

    return vec3_matches(hth_vec3_add(x, y), 1.0F, 1.0F, 0.0F) &&
           vec3_matches(hth_vec3_subtract(x, y), 1.0F, -1.0F, 0.0F) &&
           vec3_matches(hth_vec3_scale(value, 2.0F), 6.0F, 8.0F, 0.0F) &&
           close_enough(hth_vec3_dot(x, y), 0.0F) &&
           vec3_matches(hth_vec3_cross(x, y), 0.0F, 0.0F, 1.0F) &&
           close_enough(hth_vec3_length(value), 5.0F) &&
           hth_vec3_normalize(value, &normalized) &&
           vec3_matches(normalized, 0.6F, 0.8F, 0.0F) &&
           !hth_vec3_normalize(hth_vec3(0.0F, 0.0F, 0.0F),
                               &zero_normalized) &&
           vec3_matches(zero_normalized, 0.0F, 0.0F, 0.0F);
}

static bool test_identity_and_translation(void)
{
    HTHVec4 point = {2.0F, 3.0F, 4.0F, 1.0F};
    HTHMat4 identity = hth_mat4_identity();
    HTHMat4 translation = hth_mat4_translation(hth_vec3(5.0F, -2.0F, 1.0F));

    return vec4_matches(hth_mat4_transform_vec4(identity, point),
                        2.0F, 3.0F, 4.0F, 1.0F) &&
           vec4_matches(hth_mat4_transform_vec4(translation, point),
                        7.0F, 1.0F, 5.0F, 1.0F);
}

static bool test_multiplication_order(void)
{
    HTHMat4 scale = hth_mat4_identity();
    HTHMat4 translation = hth_mat4_translation(hth_vec3(1.0F, 0.0F, 0.0F));
    HTHVec4 point = {1.0F, 0.0F, 0.0F, 1.0F};
    HTHVec4 translated_after_scale;
    HTHVec4 scaled_after_translation;

    scale.elements[0] = 2.0F;
    translated_after_scale = hth_mat4_transform_vec4(
        hth_mat4_multiply(translation, scale), point);
    scaled_after_translation = hth_mat4_transform_vec4(
        hth_mat4_multiply(scale, translation), point);
    return vec4_matches(translated_after_scale, 3.0F, 0.0F, 0.0F, 1.0F) &&
           vec4_matches(scaled_after_translation, 4.0F, 0.0F, 0.0F, 1.0F);
}

static bool test_perspective(void)
{
    HTHMat4 wide;
    HTHMat4 square;
    HTHVec4 center;
    HTHVec4 near_point = {0.0F, 0.0F, -0.1F, 1.0F};
    HTHVec4 far_point = {0.0F, 0.0F, -100.0F, 1.0F};
    float fov = hth_degrees_to_radians(90.0F);

    if (!hth_mat4_perspective(fov, 1.0F, 0.1F, 100.0F, &square) ||
        !hth_mat4_perspective(fov, 2.0F, 0.1F, 100.0F, &wide)) {
        return false;
    }
    center = hth_mat4_transform_vec4(square,
                                     (HTHVec4){0.0F, 0.0F, -1.0F, 1.0F});
    near_point = hth_mat4_transform_vec4(square, near_point);
    far_point = hth_mat4_transform_vec4(square, far_point);
    return close_enough(center.x / center.w, 0.0F) &&
           close_enough(center.y / center.w, 0.0F) &&
           close_enough(near_point.z / near_point.w, -1.0F) &&
           close_enough(far_point.z / far_point.w, 1.0F) &&
           close_enough(wide.elements[0], square.elements[0] * 0.5F) &&
           !hth_mat4_perspective(0.0F, 1.0F, 0.1F, 100.0F, &wide) &&
           !hth_mat4_perspective(fov, 0.0F, 0.1F, 100.0F, &wide) &&
           !hth_mat4_perspective(fov, 1.0F, 0.0F, 100.0F, &wide) &&
           !hth_mat4_perspective(fov, 1.0F, 1.0F, 1.0F, &wide);
}

int main(void)
{
    if (!test_vectors() || !test_identity_and_translation() ||
        !test_multiplication_order() || !test_perspective()) {
        fputs("Math foundation test failed.\n", stderr);
        return 1;
    }
    puts("Math foundation tests passed.");
    return 0;
}
