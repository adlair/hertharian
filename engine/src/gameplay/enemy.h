#ifndef HTH_ENEMY_H
#define HTH_ENEMY_H

#include "actor.h"
#include "entity.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct HTHEnemyStore HTHEnemyStore;

typedef struct {
    size_t next_index;
} HTHEnemyIterator;

HTHEnemyStore *hth_enemy_store_create(void);
void hth_enemy_store_destroy(HTHEnemyStore *store);

bool hth_enemy_store_attach(HTHEnemyStore *store,
                            const HTHEntityRegistry *entities,
                            const HTHActorStore *actors,
                            HTHEntityHandle entity);
bool hth_enemy_store_has(const HTHEnemyStore *store,
                         const HTHEntityRegistry *entities,
                         const HTHActorStore *actors,
                         HTHEntityHandle entity);
bool hth_enemy_store_remove(HTHEnemyStore *store,
                            const HTHEntityRegistry *entities,
                            HTHEntityHandle entity);

void hth_enemy_iterator_begin(HTHEnemyIterator *iterator);
bool hth_enemy_iterator_next(const HTHEnemyStore *store,
                             const HTHEntityRegistry *entities,
                             const HTHActorStore *actors,
                             HTHEnemyIterator *iterator,
                             HTHEntityHandle *out_entity);

#endif
