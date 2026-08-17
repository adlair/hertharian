#include "enemy_target.h"

#include <stdint.h>
#include <stdlib.h>

#define HTH_ENEMY_TARGET_INITIAL_CAPACITY ((size_t)64U)

typedef struct {
    uint32_t owner_generation;
    HTHEntityHandle target;
    bool present;
} HTHEnemyTargetEntry;

struct HTHEnemyTargetStore {
    HTHEnemyTargetEntry *entries;
    size_t capacity;
};

static void initialize_entries(HTHEnemyTargetEntry *entries, size_t begin,
                               size_t end)
{
    size_t index;

    for (index = begin; index < end; ++index) {
        entries[index] = (HTHEnemyTargetEntry){0};
    }
}

static bool ensure_capacity(HTHEnemyTargetStore *store,
                            uint32_t entity_index)
{
    const size_t maximum_capacity = (size_t)UINT32_MAX;
    HTHEnemyTargetEntry *grown_entries;
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

static bool relation_is_valid(const HTHEnemyTargetStore *store,
                              const HTHEntityRegistry *entities,
                              const HTHActorStore *actors,
                              const HTHEnemyStore *enemies,
                              HTHEntityHandle enemy)
{
    const HTHEnemyTargetEntry *entry;

    if (store == NULL || entities == NULL || actors == NULL ||
        enemies == NULL ||
        !hth_enemy_store_has(enemies, entities, actors, enemy) ||
        (size_t)enemy.index >= store->capacity) {
        return false;
    }
    entry = &store->entries[enemy.index];
    return entry->present &&
           entry->owner_generation == enemy.generation &&
           hth_entity_registry_is_alive(entities, entry->target);
}

HTHEnemyTargetStore *hth_enemy_target_store_create(void)
{
    HTHEnemyTargetStore *store = calloc(1U, sizeof(*store));

    if (store == NULL ||
        HTH_ENEMY_TARGET_INITIAL_CAPACITY >
            SIZE_MAX / sizeof(*store->entries)) {
        free(store);
        return NULL;
    }
    store->entries = malloc(HTH_ENEMY_TARGET_INITIAL_CAPACITY *
                            sizeof(*store->entries));
    if (store->entries == NULL) {
        free(store);
        return NULL;
    }
    initialize_entries(store->entries, 0U,
                       HTH_ENEMY_TARGET_INITIAL_CAPACITY);
    store->capacity = HTH_ENEMY_TARGET_INITIAL_CAPACITY;
    return store;
}

void hth_enemy_target_store_destroy(HTHEnemyTargetStore *store)
{
    if (store != NULL) {
        free(store->entries);
        free(store);
    }
}

bool hth_enemy_target_store_set(HTHEnemyTargetStore *store,
                                const HTHEntityRegistry *entities,
                                const HTHActorStore *actors,
                                const HTHEnemyStore *enemies,
                                HTHEntityHandle enemy,
                                HTHEntityHandle target)
{
    HTHEnemyTargetEntry *entry;

    if (store == NULL || entities == NULL || actors == NULL ||
        enemies == NULL ||
        !hth_enemy_store_has(enemies, entities, actors, enemy) ||
        !hth_entity_registry_is_alive(entities, target) ||
        !ensure_capacity(store, enemy.index)) {
        return false;
    }
    entry = &store->entries[enemy.index];
    entry->owner_generation = enemy.generation;
    entry->target = target;
    entry->present = true;
    return true;
}

bool hth_enemy_target_store_has(const HTHEnemyTargetStore *store,
                                const HTHEntityRegistry *entities,
                                const HTHActorStore *actors,
                                const HTHEnemyStore *enemies,
                                HTHEntityHandle enemy)
{
    return relation_is_valid(store, entities, actors, enemies, enemy);
}

bool hth_enemy_target_store_get(const HTHEnemyTargetStore *store,
                                const HTHEntityRegistry *entities,
                                const HTHActorStore *actors,
                                const HTHEnemyStore *enemies,
                                HTHEntityHandle enemy,
                                HTHEntityHandle *out_target)
{
    if (out_target != NULL) {
        *out_target = hth_entity_handle_invalid();
    }
    if (out_target == NULL ||
        !relation_is_valid(store, entities, actors, enemies, enemy)) {
        return false;
    }
    *out_target = store->entries[enemy.index].target;
    return true;
}

bool hth_enemy_target_store_clear(HTHEnemyTargetStore *store,
                                  const HTHEntityRegistry *entities,
                                  HTHEntityHandle enemy)
{
    HTHEnemyTargetEntry *entry;

    if (store == NULL || entities == NULL ||
        !hth_entity_registry_is_alive(entities, enemy) ||
        (size_t)enemy.index >= store->capacity) {
        return false;
    }
    entry = &store->entries[enemy.index];
    if (!entry->present || entry->owner_generation != enemy.generation) {
        return false;
    }
    *entry = (HTHEnemyTargetEntry){0};
    return true;
}

void hth_enemy_target_iterator_begin(HTHEnemyTargetIterator *iterator)
{
    if (iterator != NULL) {
        iterator->next_index = 0U;
    }
}

bool hth_enemy_target_iterator_next(
    const HTHEnemyTargetStore *store,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHEnemyTargetIterator *iterator,
    HTHEnemyTargetPair *out_pair)
{
    if (out_pair != NULL) {
        out_pair->enemy = hth_entity_handle_invalid();
        out_pair->target = hth_entity_handle_invalid();
    }
    if (store == NULL || entities == NULL || actors == NULL ||
        enemies == NULL || iterator == NULL || out_pair == NULL) {
        return false;
    }
    while (iterator->next_index < store->capacity) {
        size_t index = iterator->next_index++;
        const HTHEnemyTargetEntry *entry = &store->entries[index];
        HTHEntityHandle enemy;

        if (!entry->present) {
            continue;
        }
        enemy.index = (uint32_t)index;
        enemy.generation = entry->owner_generation;
        if (!hth_enemy_store_has(enemies, entities, actors, enemy) ||
            !hth_entity_registry_is_alive(entities, entry->target)) {
            continue;
        }
        out_pair->enemy = enemy;
        out_pair->target = entry->target;
        return true;
    }
    return false;
}
