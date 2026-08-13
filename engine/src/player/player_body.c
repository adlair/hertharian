#include "player_body.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static const float default_half_width = 0.30F;
static const float default_height = 1.80F;
static const float default_eye_height = 1.60F;

bool hth_player_body_init(HTHPlayerBody *body, HTHVec3 feet_position)
{
    if (body == NULL) {
        return false;
    }
    memset(body, 0, sizeof(*body));
    body->position = feet_position;
    body->half_width = default_half_width;
    body->height = default_height;
    body->eye_height = default_eye_height;
    body->grounded = false;
    return hth_player_body_is_valid(body);
}

bool hth_player_body_is_valid(const HTHPlayerBody *body)
{
    return body != NULL && isfinite(body->position.x) &&
           isfinite(body->position.y) && isfinite(body->position.z) &&
           isfinite(body->velocity.x) && isfinite(body->velocity.y) &&
           isfinite(body->velocity.z) && isfinite(body->half_width) &&
           isfinite(body->height) && isfinite(body->eye_height) &&
           body->half_width > 0.0F && body->height > 0.0F &&
           body->eye_height > 0.0F && body->eye_height < body->height;
}

bool hth_player_body_bounds(const HTHPlayerBody *body, HTHAABB *out_bounds)
{
    if (!hth_player_body_is_valid(body) || out_bounds == NULL) {
        return false;
    }
    out_bounds->min = hth_vec3(body->position.x - body->half_width,
                               body->position.y,
                               body->position.z - body->half_width);
    out_bounds->max = hth_vec3(body->position.x + body->half_width,
                               body->position.y + body->height,
                               body->position.z + body->half_width);
    return true;
}

bool hth_player_body_eye_position(const HTHPlayerBody *body,
                                  HTHVec3 *out_position)
{
    if (!hth_player_body_is_valid(body) || out_position == NULL) {
        return false;
    }
    *out_position = hth_vec3_add(
        body->position, hth_vec3(0.0F, body->eye_height, 0.0F));
    return true;
}
