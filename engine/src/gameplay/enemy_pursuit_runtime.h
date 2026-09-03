#ifndef HTH_ENEMY_PURSUIT_RUNTIME_H
#define HTH_ENEMY_PURSUIT_RUNTIME_H

#include "actor.h"
#include "collision_world.h"
#include "dynamic_body.h"
#include "enemy.h"
#include "enemy_attack_cadence_store.h"
#include "enemy_target.h"
#include "entity.h"
#include "health.h"
#include "spatial.h"

#include <stdbool.h>
#include <stddef.h>

bool hth_enemy_pursuit_runtime_step(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    HTHEnemyTargetStore *targets,
    HTHEnemyAttackCadenceStore *cadences,
    const HTHCollisionWorld *collision_world,
    const HTHEntityHandle *candidates,
    size_t candidate_count,
    float perception_radius,
    float attack_range,
    float chase_speed,
    float attack_damage,
    double attack_interval_seconds,
    double delta_seconds);

#endif
