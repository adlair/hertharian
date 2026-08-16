#ifndef HTH_DYNAMIC_BODY_H
#define HTH_DYNAMIC_BODY_H

#include "entity.h"
#include "hth_math.h"
#include "spatial.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    HTHVec3 half_extents;
    HTHVec3 velocity;
} HTHDynamicBody;

typedef struct HTHDynamicBodyStore HTHDynamicBodyStore;

typedef struct {
    size_t next_index;
} HTHDynamicBodyIterator;

HTHDynamicBodyStore *hth_dynamic_body_store_create(void);
void hth_dynamic_body_store_destroy(HTHDynamicBodyStore *store);

bool hth_dynamic_body_attach(HTHDynamicBodyStore *store,
                             const HTHEntityRegistry *entities,
                             const HTHSpatialStore *spatial,
                             HTHEntityHandle entity,
                             const HTHDynamicBody *body);
bool hth_dynamic_body_has(const HTHDynamicBodyStore *store,
                          const HTHEntityRegistry *entities,
                          HTHEntityHandle entity);
bool hth_dynamic_body_get(const HTHDynamicBodyStore *store,
                          const HTHEntityRegistry *entities,
                          HTHEntityHandle entity,
                          HTHDynamicBody *out_body);
bool hth_dynamic_body_set_velocity(HTHDynamicBodyStore *store,
                                   const HTHEntityRegistry *entities,
                                   HTHEntityHandle entity,
                                   HTHVec3 velocity);
bool hth_dynamic_body_remove(HTHDynamicBodyStore *store,
                             const HTHEntityRegistry *entities,
                             HTHEntityHandle entity);

void hth_dynamic_body_iterator_begin(HTHDynamicBodyIterator *iterator);
bool hth_dynamic_body_iterator_next(const HTHDynamicBodyStore *store,
                                    const HTHEntityRegistry *entities,
                                    HTHDynamicBodyIterator *iterator,
                                    HTHEntityHandle *out_entity,
                                    HTHDynamicBody *out_body);

#endif
