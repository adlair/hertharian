#ifndef HTH_ACTOR_H
#define HTH_ACTOR_H

#include "entity.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct HTHActorStore HTHActorStore;

typedef struct {
    size_t next_index;
} HTHActorIterator;

HTHActorStore *hth_actor_store_create(void);
void hth_actor_store_destroy(HTHActorStore *store);

bool hth_actor_store_attach(HTHActorStore *store,
                            const HTHEntityRegistry *entities,
                            HTHEntityHandle entity);
bool hth_actor_store_has(const HTHActorStore *store,
                         const HTHEntityRegistry *entities,
                         HTHEntityHandle entity);
bool hth_actor_store_remove(HTHActorStore *store,
                            const HTHEntityRegistry *entities,
                            HTHEntityHandle entity);

void hth_actor_iterator_begin(HTHActorIterator *iterator);
bool hth_actor_iterator_next(const HTHActorStore *store,
                             const HTHEntityRegistry *entities,
                             HTHActorIterator *iterator,
                             HTHEntityHandle *out_entity);

#endif
