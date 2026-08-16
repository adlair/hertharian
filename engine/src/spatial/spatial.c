#include "spatial.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define HTH_SPATIAL_INITIAL_CAPACITY ((size_t)64U)

typedef struct {
    HTHSpatialTransform transform;
    uint32_t generation;
    bool present;
} HTHSpatialEntry;

struct HTHSpatialStore {
    HTHSpatialEntry *entries;
    size_t capacity;
};

static HTHSpatialTransform zero_transform(void)
{
    HTHSpatialTransform transform = {{0.0F, 0.0F, 0.0F}, 0.0F};

    return transform;
}

static bool transform_is_finite(const HTHSpatialTransform *transform)
{
    return transform != NULL && isfinite(transform->position.x) &&
           isfinite(transform->position.y) &&
           isfinite(transform->position.z) && isfinite(transform->yaw);
}

static void initialize_entries(HTHSpatialEntry *entries, size_t begin,
                               size_t end)
{
    size_t index;

    for (index = begin; index < end; ++index) {
        entries[index].transform = zero_transform();
        entries[index].generation = 0U;
        entries[index].present = false;
    }
}

static bool ensure_capacity(HTHSpatialStore *store, uint32_t entity_index)
{
    const size_t maximum_capacity = (size_t)UINT32_MAX;
    HTHSpatialEntry *grown_entries;
    size_t new_capacity;
    size_t old_capacity;
    size_t required_capacity;

    if (store == NULL || entity_index == UINT32_MAX) {
        return false;
    }
    required_capacity = (size_t)entity_index + 1U;
    if (required_capacity > maximum_capacity) {
        return false;
    }
    if (required_capacity <= store->capacity) {
        return true;
    }
    old_capacity = store->capacity;
    new_capacity = old_capacity;
    while (new_capacity < required_capacity) {
        if (new_capacity > maximum_capacity / 2U) {
            new_capacity = maximum_capacity;
        } else {
            new_capacity *= 2U;
        }
    }
    if (new_capacity <= old_capacity ||
        new_capacity > SIZE_MAX / sizeof(*store->entries)) {
        return false;
    }
    grown_entries = realloc(store->entries,
                            new_capacity * sizeof(*store->entries));
    if (grown_entries == NULL) {
        return false;
    }
    initialize_entries(grown_entries, old_capacity, new_capacity);
    store->entries = grown_entries;
    store->capacity = new_capacity;
    return true;
}

HTHSpatialStore *hth_spatial_store_create(void)
{
    HTHSpatialStore *store = calloc(1U, sizeof(*store));

    if (store == NULL ||
        HTH_SPATIAL_INITIAL_CAPACITY > SIZE_MAX / sizeof(*store->entries)) {
        free(store);
        return NULL;
    }
    store->entries = malloc(HTH_SPATIAL_INITIAL_CAPACITY *
                            sizeof(*store->entries));
    if (store->entries == NULL) {
        free(store);
        return NULL;
    }
    initialize_entries(store->entries, 0U, HTH_SPATIAL_INITIAL_CAPACITY);
    store->capacity = HTH_SPATIAL_INITIAL_CAPACITY;
    return store;
}

void hth_spatial_store_destroy(HTHSpatialStore *store)
{
    if (store == NULL) {
        return;
    }
    free(store->entries);
    free(store);
}

bool hth_spatial_store_attach(HTHSpatialStore *store,
                              const HTHEntityRegistry *entities,
                              HTHEntityHandle entity,
                              const HTHSpatialTransform *transform)
{
    HTHSpatialEntry *entry;

    if (store == NULL || entities == NULL ||
        !transform_is_finite(transform) ||
        !hth_entity_registry_is_alive(entities, entity)) {
        return false;
    }
    if ((size_t)entity.index < store->capacity) {
        entry = &store->entries[entity.index];
        if (entry->present && entry->generation == entity.generation) {
            return false;
        }
    }
    if (!ensure_capacity(store, entity.index)) {
        return false;
    }
    entry = &store->entries[entity.index];
    entry->transform = *transform;
    entry->generation = entity.generation;
    entry->present = true;
    return true;
}

bool hth_spatial_store_has(const HTHSpatialStore *store,
                           const HTHEntityRegistry *entities,
                           HTHEntityHandle entity)
{
    const HTHSpatialEntry *entry;

    if (store == NULL || entities == NULL ||
        !hth_entity_registry_is_alive(entities, entity) ||
        (size_t)entity.index >= store->capacity) {
        return false;
    }
    entry = &store->entries[entity.index];
    return entry->present && entry->generation == entity.generation;
}

bool hth_spatial_store_get(const HTHSpatialStore *store,
                           const HTHEntityRegistry *entities,
                           HTHEntityHandle entity,
                           HTHSpatialTransform *out_transform)
{
    if (out_transform == NULL) {
        return false;
    }
    *out_transform = zero_transform();
    if (!hth_spatial_store_has(store, entities, entity)) {
        return false;
    }
    *out_transform = store->entries[entity.index].transform;
    return true;
}

bool hth_spatial_store_set(HTHSpatialStore *store,
                           const HTHEntityRegistry *entities,
                           HTHEntityHandle entity,
                           const HTHSpatialTransform *transform)
{
    if (store == NULL || !transform_is_finite(transform) ||
        !hth_spatial_store_has(store, entities, entity)) {
        return false;
    }
    store->entries[entity.index].transform = *transform;
    return true;
}

bool hth_spatial_store_remove(HTHSpatialStore *store,
                              const HTHEntityRegistry *entities,
                              HTHEntityHandle entity)
{
    HTHSpatialEntry *entry;

    if (store == NULL || !hth_spatial_store_has(store, entities, entity)) {
        return false;
    }
    entry = &store->entries[entity.index];
    entry->transform = zero_transform();
    entry->generation = 0U;
    entry->present = false;
    return true;
}

void hth_spatial_iterator_begin(HTHSpatialIterator *iterator)
{
    if (iterator != NULL) {
        iterator->next_index = 0U;
    }
}

bool hth_spatial_iterator_next(const HTHSpatialStore *store,
                               const HTHEntityRegistry *entities,
                               HTHSpatialIterator *iterator,
                               HTHEntityHandle *out_entity,
                               HTHSpatialTransform *out_transform)
{
    if (out_entity != NULL) {
        *out_entity = hth_entity_handle_invalid();
    }
    if (out_transform != NULL) {
        *out_transform = zero_transform();
    }
    if (out_entity == NULL || out_transform == NULL || store == NULL ||
        entities == NULL || iterator == NULL) {
        return false;
    }
    while (iterator->next_index < store->capacity) {
        size_t index = iterator->next_index++;
        const HTHSpatialEntry *entry = &store->entries[index];
        HTHEntityHandle entity;

        if (!entry->present) {
            continue;
        }
        entity.index = (uint32_t)index;
        entity.generation = entry->generation;
        if (!hth_entity_registry_is_alive(entities, entity)) {
            continue;
        }
        *out_entity = entity;
        *out_transform = entry->transform;
        return true;
    }
    return false;
}
