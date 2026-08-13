#ifndef HTH_COLLISION_WORLD_H
#define HTH_COLLISION_WORLD_H

#include "aabb.h"
#include "player_body.h"

#include <stdbool.h>
#include <stddef.h>

#define HTH_COLLISION_WORLD_MAX_OBSTACLES 8

typedef struct HTHCollisionWorld {
    HTHAABB obstacles[HTH_COLLISION_WORLD_MAX_OBSTACLES];
    size_t obstacle_count;
} HTHCollisionWorld;

bool hth_collision_world_init_bootstrap(HTHCollisionWorld *world);
bool hth_collision_world_is_valid(const HTHCollisionWorld *world);
bool hth_collision_world_body_penetrates(const HTHCollisionWorld *world,
                                         const HTHPlayerBody *body);
bool hth_collision_world_move_body(const HTHCollisionWorld *world,
                                   HTHPlayerBody *body,
                                   HTHVec3 displacement);

#endif
