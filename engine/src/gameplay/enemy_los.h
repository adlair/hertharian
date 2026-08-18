#ifndef HTH_ENEMY_LOS_H
#define HTH_ENEMY_LOS_H

#include "actor.h"
#include "collision_world.h"
#include "enemy.h"
#include "entity.h"
#include "spatial.h"

#include <stdbool.h>

bool hth_enemy_los_has_line_of_sight(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    const HTHSpatialStore *spatial,
    const HTHCollisionWorld *collision_world,
    HTHEntityHandle enemy,
    HTHEntityHandle candidate);

#endif
