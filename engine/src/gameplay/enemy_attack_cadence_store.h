#ifndef HTH_ENEMY_ATTACK_CADENCE_STORE_H
#define HTH_ENEMY_ATTACK_CADENCE_STORE_H

#include "actor.h"
#include "enemy.h"
#include "enemy_attack_cadence.h"
#include "entity.h"

#include <stdbool.h>

typedef struct HTHEnemyAttackCadenceStore HTHEnemyAttackCadenceStore;

HTHEnemyAttackCadenceStore *hth_enemy_attack_cadence_store_create(void);
void hth_enemy_attack_cadence_store_destroy(
    HTHEnemyAttackCadenceStore *store);

bool hth_enemy_attack_cadence_store_attach(
    HTHEnemyAttackCadenceStore *store,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHEntityHandle enemy);
bool hth_enemy_attack_cadence_store_has(
    const HTHEnemyAttackCadenceStore *store,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHEntityHandle enemy);
bool hth_enemy_attack_cadence_store_get_mutable(
    HTHEnemyAttackCadenceStore *store,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHEntityHandle enemy,
    HTHEnemyAttackCadence **out_cadence);
bool hth_enemy_attack_cadence_store_remove(
    HTHEnemyAttackCadenceStore *store,
    const HTHEntityRegistry *entities,
    HTHEntityHandle enemy);

#endif
