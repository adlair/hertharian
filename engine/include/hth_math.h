#ifndef HTH_MATH_H
#define HTH_MATH_H

#include <stdbool.h>

typedef struct {
    float x;
    float y;
} HTHVec2;

typedef struct {
    float x;
    float y;
    float z;
} HTHVec3;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} HTHVec4;

/* Column-major storage with column vectors: transformed = matrix * vector. */
typedef struct {
    float elements[16];
} HTHMat4;

float hth_degrees_to_radians(float degrees);

HTHVec2 hth_vec2(float x, float y);
HTHVec3 hth_vec3(float x, float y, float z);
HTHVec3 hth_vec3_add(HTHVec3 left, HTHVec3 right);
HTHVec3 hth_vec3_subtract(HTHVec3 left, HTHVec3 right);
HTHVec3 hth_vec3_scale(HTHVec3 vector, float scalar);
float hth_vec3_dot(HTHVec3 left, HTHVec3 right);
HTHVec3 hth_vec3_cross(HTHVec3 left, HTHVec3 right);
float hth_vec3_length(HTHVec3 vector);
bool hth_vec3_normalize(HTHVec3 vector, HTHVec3 *out_normalized);

HTHMat4 hth_mat4_identity(void);
HTHMat4 hth_mat4_multiply(HTHMat4 left, HTHMat4 right);
HTHMat4 hth_mat4_translation(HTHVec3 translation);
bool hth_mat4_perspective(float vertical_fov_radians, float aspect,
                          float near_plane, float far_plane,
                          HTHMat4 *out_projection);
bool hth_mat4_look_direction(HTHVec3 position, HTHVec3 forward, HTHVec3 up,
                             HTHMat4 *out_view);
HTHVec4 hth_mat4_transform_vec4(HTHMat4 matrix, HTHVec4 vector);

#endif
