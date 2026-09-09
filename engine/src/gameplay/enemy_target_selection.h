#ifndef HTH_ENEMY_TARGET_SELECTION_H
#define HTH_ENEMY_TARGET_SELECTION_H

#include "actor.h"
#include "collision_world.h"
#include "enemy.h"
#include "enemy_target.h"
#include "entity.h"
#include "spatial.h"

#include <stdbool.h>
#include <stddef.h>

bool hth_enemy_target_select(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    const HTHSpatialStore *spatial,
    const HTHCollisionWorld *collision_world,
    HTHEnemyTargetStore *targets,
    HTHEntityHandle enemy,
    const HTHEntityHandle *candidates,
    size_t candidate_count,
    const HTHEntityHandle *excluded_targets,
    size_t excluded_target_count,
    float perception_radius,
    HTHEntityHandle *out_selected);

#endif
