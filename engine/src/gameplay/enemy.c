#include "enemy.h"

#include <stdint.h>
#include <stdlib.h>

#define HTH_ENEMY_INITIAL_CAPACITY ((size_t)64U)

typedef struct {
    uint32_t generation;
    bool present;
} HTHEnemyEntry;

struct HTHEnemyStore {
    HTHEnemyEntry *entries;
    size_t capacity;
};

static void initialize_entries(HTHEnemyEntry *entries, size_t begin,
                               size_t end)
{
    size_t index;

    for (index = begin; index < end; ++index) {
        entries[index] = (HTHEnemyEntry){0};
    }
}

static bool ensure_capacity(HTHEnemyStore *store, uint32_t entity_index)
{
    const size_t maximum_capacity = (size_t)UINT32_MAX;
    HTHEnemyEntry *grown_entries;
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

HTHEnemyStore *hth_enemy_store_create(void)
{
    HTHEnemyStore *store = calloc(1U, sizeof(*store));

    if (store == NULL || HTH_ENEMY_INITIAL_CAPACITY >
                             SIZE_MAX / sizeof(*store->entries)) {
        free(store);
        return NULL;
    }
    store->entries = malloc(HTH_ENEMY_INITIAL_CAPACITY *
                            sizeof(*store->entries));
    if (store->entries == NULL) {
        free(store);
        return NULL;
    }
    initialize_entries(store->entries, 0U, HTH_ENEMY_INITIAL_CAPACITY);
    store->capacity = HTH_ENEMY_INITIAL_CAPACITY;
    return store;
}

void hth_enemy_store_destroy(HTHEnemyStore *store)
{
    if (store != NULL) {
        free(store->entries);
        free(store);
    }
}

bool hth_enemy_store_has(const HTHEnemyStore *store,
                         const HTHEntityRegistry *entities,
                         const HTHActorStore *actors,
                         HTHEntityHandle entity)
{
    const HTHEnemyEntry *entry;

    if (store == NULL || entities == NULL || actors == NULL ||
        !hth_actor_store_has(actors, entities, entity) ||
        (size_t)entity.index >= store->capacity) {
        return false;
    }
    entry = &store->entries[entity.index];
    return entry->present && entry->generation == entity.generation;
}

bool hth_enemy_store_attach(HTHEnemyStore *store,
                            const HTHEntityRegistry *entities,
                            const HTHActorStore *actors,
                            HTHEntityHandle entity)
{
    HTHEnemyEntry *entry;

    if (store == NULL || entities == NULL || actors == NULL ||
        !hth_actor_store_has(actors, entities, entity)) {
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
    entry->generation = entity.generation;
    entry->present = true;
    return true;
}

bool hth_enemy_store_remove(HTHEnemyStore *store,
                            const HTHEntityRegistry *entities,
                            HTHEntityHandle entity)
{
    HTHEnemyEntry *entry;

    if (store == NULL || entities == NULL ||
        !hth_entity_registry_is_alive(entities, entity) ||
        (size_t)entity.index >= store->capacity) {
        return false;
    }
    entry = &store->entries[entity.index];
    if (!entry->present || entry->generation != entity.generation) {
        return false;
    }
    *entry = (HTHEnemyEntry){0};
    return true;
}

void hth_enemy_iterator_begin(HTHEnemyIterator *iterator)
{
    if (iterator != NULL) {
        iterator->next_index = 0U;
    }
}

bool hth_enemy_iterator_next(const HTHEnemyStore *store,
                             const HTHEntityRegistry *entities,
                             const HTHActorStore *actors,
                             HTHEnemyIterator *iterator,
                             HTHEntityHandle *out_entity)
{
    if (out_entity != NULL) {
        *out_entity = hth_entity_handle_invalid();
    }
    if (store == NULL || entities == NULL || actors == NULL ||
        iterator == NULL || out_entity == NULL) {
        return false;
    }
    while (iterator->next_index < store->capacity) {
        size_t index = iterator->next_index++;
        const HTHEnemyEntry *entry = &store->entries[index];
        HTHEntityHandle entity;

        if (!entry->present) {
            continue;
        }
        entity.index = (uint32_t)index;
        entity.generation = entry->generation;
        if (!hth_actor_store_has(actors, entities, entity)) {
            continue;
        }
        *out_entity = entity;
        return true;
    }
    return false;
}
