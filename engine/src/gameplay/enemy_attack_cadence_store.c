#include "enemy_attack_cadence_store.h"

#include <stdint.h>
#include <stdlib.h>

#define HTH_ENEMY_ATTACK_CADENCE_INITIAL_CAPACITY ((size_t)64U)

typedef struct {
    uint32_t generation;
    HTHEnemyAttackCadence cadence;
    bool present;
} HTHEnemyAttackCadenceEntry;

struct HTHEnemyAttackCadenceStore {
    HTHEnemyAttackCadenceEntry *entries;
    size_t capacity;
};

static void initialize_entries(HTHEnemyAttackCadenceEntry *entries,
                               size_t begin, size_t end)
{
    size_t index;

    for (index = begin; index < end; ++index) {
        entries[index] = (HTHEnemyAttackCadenceEntry){0};
    }
}

static bool ensure_capacity(HTHEnemyAttackCadenceStore *store,
                            uint32_t entity_index)
{
    const size_t maximum_capacity = (size_t)UINT32_MAX;
    HTHEnemyAttackCadenceEntry *grown_entries;
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
        new_capacity = new_capacity > maximum_capacity / 2U
            ? maximum_capacity
            : new_capacity * 2U;
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

static bool association_is_valid(
    const HTHEnemyAttackCadenceStore *store,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHEntityHandle enemy)
{
    const HTHEnemyAttackCadenceEntry *entry;

    if (store == NULL || entities == NULL || actors == NULL ||
        enemies == NULL ||
        !hth_enemy_store_has(enemies, entities, actors, enemy) ||
        (size_t)enemy.index >= store->capacity) {
        return false;
    }
    entry = &store->entries[enemy.index];
    return entry->present && entry->generation == enemy.generation;
}

HTHEnemyAttackCadenceStore *hth_enemy_attack_cadence_store_create(void)
{
    HTHEnemyAttackCadenceStore *store = calloc(1U, sizeof(*store));

    if (store == NULL || HTH_ENEMY_ATTACK_CADENCE_INITIAL_CAPACITY >
                             SIZE_MAX / sizeof(*store->entries)) {
        free(store);
        return NULL;
    }
    store->entries = malloc(HTH_ENEMY_ATTACK_CADENCE_INITIAL_CAPACITY *
                            sizeof(*store->entries));
    if (store->entries == NULL) {
        free(store);
        return NULL;
    }
    initialize_entries(store->entries, 0U,
                       HTH_ENEMY_ATTACK_CADENCE_INITIAL_CAPACITY);
    store->capacity = HTH_ENEMY_ATTACK_CADENCE_INITIAL_CAPACITY;
    return store;
}

void hth_enemy_attack_cadence_store_destroy(
    HTHEnemyAttackCadenceStore *store)
{
    if (store != NULL) {
        free(store->entries);
        free(store);
    }
}

bool hth_enemy_attack_cadence_store_attach(
    HTHEnemyAttackCadenceStore *store,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHEntityHandle enemy)
{
    HTHEnemyAttackCadenceEntry *entry;

    if (store == NULL || entities == NULL || actors == NULL ||
        enemies == NULL ||
        !hth_enemy_store_has(enemies, entities, actors, enemy)) {
        return false;
    }
    if ((size_t)enemy.index < store->capacity) {
        entry = &store->entries[enemy.index];
        if (entry->present && entry->generation == enemy.generation) {
            return false;
        }
    }
    if (!ensure_capacity(store, enemy.index)) {
        return false;
    }
    entry = &store->entries[enemy.index];
    *entry = (HTHEnemyAttackCadenceEntry){0};
    entry->generation = enemy.generation;
    hth_enemy_attack_cadence_reset(&entry->cadence);
    entry->present = true;
    return true;
}

bool hth_enemy_attack_cadence_store_has(
    const HTHEnemyAttackCadenceStore *store,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHEntityHandle enemy)
{
    return association_is_valid(store, entities, actors, enemies, enemy);
}

bool hth_enemy_attack_cadence_store_get_mutable(
    HTHEnemyAttackCadenceStore *store,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHEntityHandle enemy,
    HTHEnemyAttackCadence **out_cadence)
{
    if (out_cadence != NULL) {
        *out_cadence = NULL;
    }
    if (out_cadence == NULL ||
        !association_is_valid(store, entities, actors, enemies, enemy)) {
        return false;
    }
    *out_cadence = &store->entries[enemy.index].cadence;
    return true;
}

bool hth_enemy_attack_cadence_store_remove(
    HTHEnemyAttackCadenceStore *store,
    const HTHEntityRegistry *entities,
    HTHEntityHandle enemy)
{
    HTHEnemyAttackCadenceEntry *entry;

    if (store == NULL || entities == NULL ||
        !hth_entity_registry_is_alive(entities, enemy) ||
        (size_t)enemy.index >= store->capacity) {
        return false;
    }
    entry = &store->entries[enemy.index];
    if (!entry->present || entry->generation != enemy.generation) {
        return false;
    }
    *entry = (HTHEnemyAttackCadenceEntry){0};
    return true;
}
