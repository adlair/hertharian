#include "dynamic_body.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define HTH_DYNAMIC_BODY_INITIAL_CAPACITY ((size_t)64U)

typedef struct {
    HTHDynamicBody body;
    uint32_t generation;
    bool present;
} HTHDynamicBodyEntry;

struct HTHDynamicBodyStore {
    HTHDynamicBodyEntry *entries;
    size_t capacity;
};

static HTHDynamicBody zero_body(void)
{
    HTHDynamicBody body = {{0.0F, 0.0F, 0.0F},
                           {0.0F, 0.0F, 0.0F}};

    return body;
}

static bool vector_is_finite(HTHVec3 vector)
{
    return isfinite(vector.x) && isfinite(vector.y) && isfinite(vector.z);
}

static bool body_is_valid(const HTHDynamicBody *body)
{
    return body != NULL && vector_is_finite(body->half_extents) &&
           body->half_extents.x > 0.0F && body->half_extents.y > 0.0F &&
           body->half_extents.z > 0.0F && vector_is_finite(body->velocity);
}

static void initialize_entries(HTHDynamicBodyEntry *entries, size_t begin,
                               size_t end)
{
    size_t index;

    for (index = begin; index < end; ++index) {
        entries[index].body = zero_body();
        entries[index].generation = 0U;
        entries[index].present = false;
    }
}

static bool ensure_capacity(HTHDynamicBodyStore *store,
                            uint32_t entity_index)
{
    const size_t maximum_capacity = (size_t)UINT32_MAX;
    HTHDynamicBodyEntry *grown_entries;
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

HTHDynamicBodyStore *hth_dynamic_body_store_create(void)
{
    HTHDynamicBodyStore *store = calloc(1U, sizeof(*store));

    if (store == NULL || HTH_DYNAMIC_BODY_INITIAL_CAPACITY >
                             SIZE_MAX / sizeof(*store->entries)) {
        free(store);
        return NULL;
    }
    store->entries = malloc(HTH_DYNAMIC_BODY_INITIAL_CAPACITY *
                            sizeof(*store->entries));
    if (store->entries == NULL) {
        free(store);
        return NULL;
    }
    initialize_entries(store->entries, 0U,
                       HTH_DYNAMIC_BODY_INITIAL_CAPACITY);
    store->capacity = HTH_DYNAMIC_BODY_INITIAL_CAPACITY;
    return store;
}

void hth_dynamic_body_store_destroy(HTHDynamicBodyStore *store)
{
    if (store == NULL) {
        return;
    }
    free(store->entries);
    free(store);
}

bool hth_dynamic_body_attach(HTHDynamicBodyStore *store,
                             const HTHEntityRegistry *entities,
                             const HTHSpatialStore *spatial,
                             HTHEntityHandle entity,
                             const HTHDynamicBody *body)
{
    HTHDynamicBodyEntry *entry;

    if (store == NULL || entities == NULL || spatial == NULL ||
        !body_is_valid(body) ||
        !hth_entity_registry_is_alive(entities, entity) ||
        !hth_spatial_store_has(spatial, entities, entity)) {
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
    entry->body = *body;
    entry->generation = entity.generation;
    entry->present = true;
    return true;
}

bool hth_dynamic_body_has(const HTHDynamicBodyStore *store,
                          const HTHEntityRegistry *entities,
                          HTHEntityHandle entity)
{
    const HTHDynamicBodyEntry *entry;

    if (store == NULL || entities == NULL ||
        !hth_entity_registry_is_alive(entities, entity) ||
        (size_t)entity.index >= store->capacity) {
        return false;
    }
    entry = &store->entries[entity.index];
    return entry->present && entry->generation == entity.generation;
}

bool hth_dynamic_body_get(const HTHDynamicBodyStore *store,
                          const HTHEntityRegistry *entities,
                          HTHEntityHandle entity,
                          HTHDynamicBody *out_body)
{
    if (out_body == NULL) {
        return false;
    }
    *out_body = zero_body();
    if (!hth_dynamic_body_has(store, entities, entity)) {
        return false;
    }
    *out_body = store->entries[entity.index].body;
    return true;
}

bool hth_dynamic_body_set_velocity(HTHDynamicBodyStore *store,
                                   const HTHEntityRegistry *entities,
                                   HTHEntityHandle entity,
                                   HTHVec3 velocity)
{
    if (store == NULL || !vector_is_finite(velocity) ||
        !hth_dynamic_body_has(store, entities, entity)) {
        return false;
    }
    store->entries[entity.index].body.velocity = velocity;
    return true;
}

bool hth_dynamic_body_remove(HTHDynamicBodyStore *store,
                             const HTHEntityRegistry *entities,
                             HTHEntityHandle entity)
{
    HTHDynamicBodyEntry *entry;

    if (store == NULL || !hth_dynamic_body_has(store, entities, entity)) {
        return false;
    }
    entry = &store->entries[entity.index];
    entry->body = zero_body();
    entry->generation = 0U;
    entry->present = false;
    return true;
}

void hth_dynamic_body_iterator_begin(HTHDynamicBodyIterator *iterator)
{
    if (iterator != NULL) {
        iterator->next_index = 0U;
    }
}

bool hth_dynamic_body_iterator_next(const HTHDynamicBodyStore *store,
                                    const HTHEntityRegistry *entities,
                                    HTHDynamicBodyIterator *iterator,
                                    HTHEntityHandle *out_entity,
                                    HTHDynamicBody *out_body)
{
    if (out_entity != NULL) {
        *out_entity = hth_entity_handle_invalid();
    }
    if (out_body != NULL) {
        *out_body = zero_body();
    }
    if (store == NULL || entities == NULL || iterator == NULL ||
        out_entity == NULL || out_body == NULL) {
        return false;
    }
    while (iterator->next_index < store->capacity) {
        size_t index = iterator->next_index++;
        const HTHDynamicBodyEntry *entry = &store->entries[index];
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
        *out_body = entry->body;
        return true;
    }
    return false;
}
