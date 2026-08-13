#ifndef HTH_PLAYER_BODY_H
#define HTH_PLAYER_BODY_H

#include "aabb.h"
#include "hth_math.h"

#include <stdbool.h>

typedef struct HTHPlayerBody {
    HTHVec3 position;
    HTHVec3 velocity;
    float half_width;
    float height;
    float eye_height;
    bool grounded;
} HTHPlayerBody;

bool hth_player_body_init(HTHPlayerBody *body, HTHVec3 feet_position);
bool hth_player_body_is_valid(const HTHPlayerBody *body);
bool hth_player_body_bounds(const HTHPlayerBody *body, HTHAABB *out_bounds);
bool hth_player_body_eye_position(const HTHPlayerBody *body,
                                  HTHVec3 *out_position);

#endif
