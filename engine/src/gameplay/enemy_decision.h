#ifndef HTH_ENEMY_DECISION_H
#define HTH_ENEMY_DECISION_H

#include "actor.h"
#include "collision_world.h"
#include "enemy.h"
#include "enemy_target.h"
#include "entity.h"
#include "spatial.h"

#include <stdbool.h>

typedef enum HTHEnemyIntentKind {
    HTH_ENEMY_INTENT_IDLE = 0,
    HTH_ENEMY_INTENT_PURSUE
} HTHEnemyIntentKind;

typedef struct HTHEnemyIntent {
    HTHEnemyIntentKind kind;
    HTHEntityHandle target;
} HTHEnemyIntent;

bool hth_enemy_decision_evaluate(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    const HTHEnemyTargetStore *targets,
    const HTHSpatialStore *spatial,
    const HTHCollisionWorld *collision_world,
    HTHEntityHandle enemy,
    float perception_radius,
    HTHEnemyIntent *out_intent);

#endif
