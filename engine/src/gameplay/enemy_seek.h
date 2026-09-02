#ifndef HTH_ENEMY_SEEK_H
#define HTH_ENEMY_SEEK_H

#include "actor.h"
#include "enemy.h"
#include "entity.h"
#include "hth_math.h"
#include "spatial.h"

#include <stdbool.h>

bool hth_enemy_seek_compute(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    const HTHSpatialStore *spatial,
    HTHEntityHandle enemy,
    HTHEntityHandle target,
    HTHVec3 *out_direction);

#endif
