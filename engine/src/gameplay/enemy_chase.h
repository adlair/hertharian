#ifndef HTH_ENEMY_CHASE_H
#define HTH_ENEMY_CHASE_H

#include "actor.h"
#include "dynamic_collision.h"
#include "enemy.h"
#include "entity.h"
#include "hth_math.h"
#include "spatial.h"

#include <stdbool.h>

bool hth_enemy_chase_apply(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    const HTHCollisionWorld *collision_world,
    HTHEntityHandle enemy,
    HTHVec3 desired_direction,
    float chase_speed,
    float delta_seconds,
    HTHDynamicCollisionResult *out_result);

#endif
