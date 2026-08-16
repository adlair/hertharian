#ifndef HTH_HEALTH_H
#define HTH_HEALTH_H

#include "actor.h"
#include "entity.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    float current;
    float maximum;
} HTHHealth;

typedef struct {
    float previous;
    float current;
    float applied;
    bool became_zero;
} HTHDamageResult;

typedef struct {
    float previous;
    float current;
    float applied;
} HTHHealingResult;

typedef struct HTHHealthStore HTHHealthStore;

typedef struct {
    size_t next_index;
} HTHHealthIterator;

bool hth_health_is_valid(HTHHealth health);

HTHHealthStore *hth_health_store_create(void);
void hth_health_store_destroy(HTHHealthStore *store);

bool hth_health_store_attach(HTHHealthStore *store,
                             const HTHEntityRegistry *entities,
                             const HTHActorStore *actors,
                             HTHEntityHandle entity,
                             HTHHealth health);
bool hth_health_store_has(const HTHHealthStore *store,
                          const HTHEntityRegistry *entities,
                          const HTHActorStore *actors,
                          HTHEntityHandle entity);
bool hth_health_store_get(const HTHHealthStore *store,
                          const HTHEntityRegistry *entities,
                          const HTHActorStore *actors,
                          HTHEntityHandle entity,
                          HTHHealth *out_health);
bool hth_health_store_remove(HTHHealthStore *store,
                             const HTHEntityRegistry *entities,
                             const HTHActorStore *actors,
                             HTHEntityHandle entity);

bool hth_health_store_apply_damage(HTHHealthStore *store,
                                   const HTHEntityRegistry *entities,
                                   const HTHActorStore *actors,
                                   HTHEntityHandle entity,
                                   float amount,
                                   HTHDamageResult *out_result);
bool hth_health_store_apply_healing(HTHHealthStore *store,
                                    const HTHEntityRegistry *entities,
                                    const HTHActorStore *actors,
                                    HTHEntityHandle entity,
                                    float amount,
                                    HTHHealingResult *out_result);

void hth_health_iterator_begin(HTHHealthIterator *iterator);
bool hth_health_iterator_next(const HTHHealthStore *store,
                              const HTHEntityRegistry *entities,
                              const HTHActorStore *actors,
                              HTHHealthIterator *iterator,
                              HTHEntityHandle *out_entity,
                              HTHHealth *out_health);

#endif
