#ifndef HTH_DYNAMIC_COLLISION_H
#define HTH_DYNAMIC_COLLISION_H

#include "collision_world.h"
#include "dynamic_body.h"

#include <stdbool.h>

typedef struct {
    bool moved;
    bool collided;
    bool start_solid;
} HTHDynamicCollisionResult;

bool hth_dynamic_collision_move(HTHDynamicBodyStore *bodies,
                                const HTHEntityRegistry *entities,
                                HTHSpatialStore *spatial,
                                const HTHCollisionWorld *collision_world,
                                HTHEntityHandle entity,
                                float dt,
                                HTHDynamicCollisionResult *out_result);

#endif
