#ifndef HTH_ENEMY_ATTACK_ELIGIBILITY_H
#define HTH_ENEMY_ATTACK_ELIGIBILITY_H

#include "actor.h"
#include "collision_world.h"
#include "enemy.h"
#include "entity.h"
#include "spatial.h"

#include <stdbool.h>

bool hth_enemy_attack_eligibility_evaluate(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    const HTHSpatialStore *spatial,
    const HTHCollisionWorld *collision_world,
    HTHEntityHandle enemy,
    HTHEntityHandle target,
    float attack_range,
    bool *out_eligible);

#endif
