#ifndef HTH_ENEMY_PERCEPTION_H
#define HTH_ENEMY_PERCEPTION_H

#include "actor.h"
#include "enemy.h"
#include "entity.h"
#include "spatial.h"

#include <stdbool.h>

bool hth_enemy_perception_can_perceive(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    const HTHSpatialStore *spatial,
    HTHEntityHandle enemy,
    HTHEntityHandle candidate,
    float radius);

#endif
