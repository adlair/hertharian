#include "health.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define HTH_HEALTH_INITIAL_CAPACITY ((size_t)64U)

typedef struct {
    HTHHealth health;
    uint32_t generation;
    bool present;
} HTHHealthEntry;

struct HTHHealthStore {
    HTHHealthEntry *entries;
    size_t capacity;
};

static void initialize_entries(HTHHealthEntry *entries, size_t begin,
                               size_t end)
{
    size_t index;

    for (index = begin; index < end; ++index) {
        entries[index] = (HTHHealthEntry){0};
    }
}

static bool ensure_capacity(HTHHealthStore *store, uint32_t entity_index)
{
    const size_t maximum_capacity = (size_t)UINT32_MAX;
    HTHHealthEntry *grown_entries;
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

bool hth_health_is_valid(HTHHealth health)
{
    return isfinite(health.current) && isfinite(health.maximum) &&
           health.maximum > 0.0F && health.current >= 0.0F &&
           health.current <= health.maximum;
}

HTHHealthStore *hth_health_store_create(void)
{
    HTHHealthStore *store = calloc(1U, sizeof(*store));

    if (store == NULL || HTH_HEALTH_INITIAL_CAPACITY >
                             SIZE_MAX / sizeof(*store->entries)) {
        free(store);
        return NULL;
    }
    store->entries = malloc(HTH_HEALTH_INITIAL_CAPACITY *
                            sizeof(*store->entries));
    if (store->entries == NULL) {
        free(store);
        return NULL;
    }
    initialize_entries(store->entries, 0U, HTH_HEALTH_INITIAL_CAPACITY);
    store->capacity = HTH_HEALTH_INITIAL_CAPACITY;
    return store;
}

void hth_health_store_destroy(HTHHealthStore *store)
{
    if (store != NULL) {
        free(store->entries);
        free(store);
    }
}

bool hth_health_store_has(const HTHHealthStore *store,
                          const HTHEntityRegistry *entities,
                          const HTHActorStore *actors,
                          HTHEntityHandle entity)
{
    const HTHHealthEntry *entry;

    if (store == NULL || entities == NULL || actors == NULL ||
        !hth_actor_store_has(actors, entities, entity) ||
        (size_t)entity.index >= store->capacity) {
        return false;
    }
    entry = &store->entries[entity.index];
    return entry->present && entry->generation == entity.generation;
}

bool hth_health_store_attach(HTHHealthStore *store,
                             const HTHEntityRegistry *entities,
                             const HTHActorStore *actors,
                             HTHEntityHandle entity,
                             HTHHealth health)
{
    HTHHealthEntry *entry;

    if (store == NULL || entities == NULL || actors == NULL ||
        !hth_health_is_valid(health) ||
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
    entry->health = health;
    entry->generation = entity.generation;
    entry->present = true;
    return true;
}

bool hth_health_store_get(const HTHHealthStore *store,
                          const HTHEntityRegistry *entities,
                          const HTHActorStore *actors,
                          HTHEntityHandle entity,
                          HTHHealth *out_health)
{
    if (out_health != NULL) {
        *out_health = (HTHHealth){0};
    }
    if (out_health == NULL ||
        !hth_health_store_has(store, entities, actors, entity)) {
        return false;
    }
    *out_health = store->entries[entity.index].health;
    return true;
}

bool hth_health_store_remove(HTHHealthStore *store,
                             const HTHEntityRegistry *entities,
                             const HTHActorStore *actors,
                             HTHEntityHandle entity)
{
    if (!hth_health_store_has(store, entities, actors, entity)) {
        return false;
    }
    store->entries[entity.index] = (HTHHealthEntry){0};
    return true;
}

bool hth_health_store_apply_damage(HTHHealthStore *store,
                                   const HTHEntityRegistry *entities,
                                   const HTHActorStore *actors,
                                   HTHEntityHandle entity,
                                   float amount,
                                   HTHDamageResult *out_result)
{
    HTHHealthEntry *entry;
    float previous;
    float current;

    if (out_result != NULL) {
        *out_result = (HTHDamageResult){0};
    }
    if (out_result == NULL || !isfinite(amount) || amount < 0.0F ||
        !hth_health_store_has(store, entities, actors, entity)) {
        return false;
    }
    entry = &store->entries[entity.index];
    previous = entry->health.current;
    current = amount >= previous ? 0.0F : previous - amount;
    entry->health.current = current;
    out_result->previous = previous;
    out_result->current = current;
    out_result->applied = previous - current;
    out_result->became_zero = previous > 0.0F && current == 0.0F;
    return true;
}

bool hth_health_store_apply_healing(HTHHealthStore *store,
                                    const HTHEntityRegistry *entities,
                                    const HTHActorStore *actors,
                                    HTHEntityHandle entity,
                                    float amount,
                                    HTHHealingResult *out_result)
{
    HTHHealthEntry *entry;
    float previous;
    float remaining;
    float current;

    if (out_result != NULL) {
        *out_result = (HTHHealingResult){0};
    }
    if (out_result == NULL || !isfinite(amount) || amount < 0.0F ||
        !hth_health_store_has(store, entities, actors, entity)) {
        return false;
    }
    entry = &store->entries[entity.index];
    previous = entry->health.current;
    remaining = entry->health.maximum - previous;
    if (amount >= remaining) {
        current = entry->health.maximum;
    } else {
        current = previous + amount;
        if (!isfinite(current) || current > entry->health.maximum) {
            current = entry->health.maximum;
        }
    }
    entry->health.current = current;
    out_result->previous = previous;
    out_result->current = current;
    out_result->applied = current - previous;
    return true;
}

void hth_health_iterator_begin(HTHHealthIterator *iterator)
{
    if (iterator != NULL) {
        iterator->next_index = 0U;
    }
}

bool hth_health_iterator_next(const HTHHealthStore *store,
                              const HTHEntityRegistry *entities,
                              const HTHActorStore *actors,
                              HTHHealthIterator *iterator,
                              HTHEntityHandle *out_entity,
                              HTHHealth *out_health)
{
    if (out_entity != NULL) {
        *out_entity = hth_entity_handle_invalid();
    }
    if (out_health != NULL) {
        *out_health = (HTHHealth){0};
    }
    if (store == NULL || entities == NULL || actors == NULL ||
        iterator == NULL || out_entity == NULL || out_health == NULL) {
        return false;
    }
    while (iterator->next_index < store->capacity) {
        size_t index = iterator->next_index++;
        const HTHHealthEntry *entry = &store->entries[index];
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
        *out_health = entry->health;
        return true;
    }
    return false;
}
