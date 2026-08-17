#ifndef HTH_ENEMY_TARGET_H
#define HTH_ENEMY_TARGET_H

#include "actor.h"
#include "enemy.h"
#include "entity.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct HTHEnemyTargetStore HTHEnemyTargetStore;

typedef struct {
    HTHEntityHandle enemy;
    HTHEntityHandle target;
} HTHEnemyTargetPair;

typedef struct {
    size_t next_index;
} HTHEnemyTargetIterator;

HTHEnemyTargetStore *hth_enemy_target_store_create(void);
void hth_enemy_target_store_destroy(HTHEnemyTargetStore *store);

bool hth_enemy_target_store_set(HTHEnemyTargetStore *store,
                                const HTHEntityRegistry *entities,
                                const HTHActorStore *actors,
                                const HTHEnemyStore *enemies,
                                HTHEntityHandle enemy,
                                HTHEntityHandle target);
bool hth_enemy_target_store_has(const HTHEnemyTargetStore *store,
                                const HTHEntityRegistry *entities,
                                const HTHActorStore *actors,
                                const HTHEnemyStore *enemies,
                                HTHEntityHandle enemy);
bool hth_enemy_target_store_get(const HTHEnemyTargetStore *store,
                                const HTHEntityRegistry *entities,
                                const HTHActorStore *actors,
                                const HTHEnemyStore *enemies,
                                HTHEntityHandle enemy,
                                HTHEntityHandle *out_target);
bool hth_enemy_target_store_clear(HTHEnemyTargetStore *store,
                                  const HTHEntityRegistry *entities,
                                  HTHEntityHandle enemy);

void hth_enemy_target_iterator_begin(HTHEnemyTargetIterator *iterator);
bool hth_enemy_target_iterator_next(
    const HTHEnemyTargetStore *store,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHEnemyTargetIterator *iterator,
    HTHEnemyTargetPair *out_pair);

#endif
