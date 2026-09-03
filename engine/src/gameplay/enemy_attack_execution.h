#ifndef HTH_ENEMY_ATTACK_EXECUTION_H
#define HTH_ENEMY_ATTACK_EXECUTION_H

#include "actor.h"
#include "damage_intent.h"
#include "enemy.h"
#include "entity.h"

#include <stdbool.h>

bool hth_enemy_attack_build_damage_intent(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHEntityHandle enemy,
    HTHEntityHandle target,
    float damage,
    HTHDamageIntent *out_intent);

#endif
