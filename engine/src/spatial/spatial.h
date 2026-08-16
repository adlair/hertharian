#ifndef HTH_SPATIAL_H
#define HTH_SPATIAL_H

#include "entity.h"
#include "hth_math.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    HTHVec3 position;
    float yaw;
} HTHSpatialTransform;

typedef struct HTHSpatialStore HTHSpatialStore;

typedef struct {
    size_t next_index;
} HTHSpatialIterator;

HTHSpatialStore *hth_spatial_store_create(void);
void hth_spatial_store_destroy(HTHSpatialStore *store);

bool hth_spatial_store_attach(HTHSpatialStore *store,
                              const HTHEntityRegistry *entities,
                              HTHEntityHandle entity,
                              const HTHSpatialTransform *transform);
bool hth_spatial_store_has(const HTHSpatialStore *store,
                           const HTHEntityRegistry *entities,
                           HTHEntityHandle entity);
bool hth_spatial_store_get(const HTHSpatialStore *store,
                           const HTHEntityRegistry *entities,
                           HTHEntityHandle entity,
                           HTHSpatialTransform *out_transform);
bool hth_spatial_store_set(HTHSpatialStore *store,
                           const HTHEntityRegistry *entities,
                           HTHEntityHandle entity,
                           const HTHSpatialTransform *transform);
bool hth_spatial_store_remove(HTHSpatialStore *store,
                              const HTHEntityRegistry *entities,
                              HTHEntityHandle entity);

void hth_spatial_iterator_begin(HTHSpatialIterator *iterator);
bool hth_spatial_iterator_next(const HTHSpatialStore *store,
                               const HTHEntityRegistry *entities,
                               HTHSpatialIterator *iterator,
                               HTHEntityHandle *out_entity,
                               HTHSpatialTransform *out_transform);

#endif
