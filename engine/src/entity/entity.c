#include "entity.h"

#include <stdint.h>
#include <stdlib.h>

#define HTH_ENTITY_INITIAL_CAPACITY ((size_t)64U)
#define HTH_ENTITY_FREE_LIST_END UINT32_MAX

typedef enum {
    HTH_ENTITY_SLOT_FREE = 0,
    HTH_ENTITY_SLOT_ALIVE,
    HTH_ENTITY_SLOT_RETIRED
} HTHEntitySlotState;

typedef struct {
    uint32_t generation;
    uint32_t next_free;
    HTHEntitySlotState state;
} HTHEntitySlot;

struct HTHEntityRegistry {
    HTHEntitySlot *slots;
    size_t capacity;
    size_t live_count;
    uint32_t free_head;
};

static void initialize_free_slots(HTHEntitySlot *slots, size_t begin,
                                  size_t end)
{
    size_t index;

    for (index = begin; index < end; ++index) {
        slots[index].generation = 1U;
        slots[index].next_free = index + 1U < end
            ? (uint32_t)(index + 1U)
            : HTH_ENTITY_FREE_LIST_END;
        slots[index].state = HTH_ENTITY_SLOT_FREE;
    }
}

static bool grow_registry(HTHEntityRegistry *registry)
{
    const size_t maximum_capacity = (size_t)UINT32_MAX;
    HTHEntitySlot *grown_slots;
    size_t new_capacity;
    size_t old_capacity;

    if (registry == NULL || registry->free_head != HTH_ENTITY_FREE_LIST_END ||
        registry->capacity >= maximum_capacity) {
        return false;
    }
    old_capacity = registry->capacity;
    if (old_capacity > maximum_capacity / 2U) {
        new_capacity = maximum_capacity;
    } else {
        new_capacity = old_capacity * 2U;
    }
    if (new_capacity <= old_capacity ||
        new_capacity > SIZE_MAX / sizeof(*registry->slots)) {
        return false;
    }
    grown_slots = realloc(registry->slots,
                          new_capacity * sizeof(*registry->slots));
    if (grown_slots == NULL) {
        return false;
    }
    initialize_free_slots(grown_slots, old_capacity, new_capacity);
    registry->slots = grown_slots;
    registry->capacity = new_capacity;
    registry->free_head = (uint32_t)old_capacity;
    return true;
}

HTHEntityHandle hth_entity_handle_invalid(void)
{
    HTHEntityHandle handle = {UINT32_MAX, 0U};

    return handle;
}

bool hth_entity_handle_equal(HTHEntityHandle left, HTHEntityHandle right)
{
    return left.index == right.index && left.generation == right.generation;
}

HTHEntityRegistry *hth_entity_registry_create(void)
{
    HTHEntityRegistry *registry = calloc(1U, sizeof(*registry));

    if (registry == NULL ||
        HTH_ENTITY_INITIAL_CAPACITY > SIZE_MAX / sizeof(*registry->slots)) {
        free(registry);
        return NULL;
    }
    registry->slots = malloc(HTH_ENTITY_INITIAL_CAPACITY *
                             sizeof(*registry->slots));
    if (registry->slots == NULL) {
        free(registry);
        return NULL;
    }
    initialize_free_slots(registry->slots, 0U,
                          HTH_ENTITY_INITIAL_CAPACITY);
    registry->capacity = HTH_ENTITY_INITIAL_CAPACITY;
    registry->free_head = 0U;
    return registry;
}

void hth_entity_registry_destroy(HTHEntityRegistry *registry)
{
    if (registry == NULL) {
        return;
    }
    free(registry->slots);
    free(registry);
}

bool hth_entity_registry_create_entity(HTHEntityRegistry *registry,
                                       HTHEntityHandle *out_handle)
{
    HTHEntitySlot *slot;
    uint32_t index;

    if (out_handle == NULL) {
        return false;
    }
    *out_handle = hth_entity_handle_invalid();
    if (registry == NULL || registry->live_count == SIZE_MAX) {
        return false;
    }
    if (registry->free_head == HTH_ENTITY_FREE_LIST_END &&
        !grow_registry(registry)) {
        return false;
    }
    index = registry->free_head;
    if ((size_t)index >= registry->capacity) {
        return false;
    }
    slot = &registry->slots[index];
    if (slot->state != HTH_ENTITY_SLOT_FREE || slot->generation == 0U) {
        return false;
    }
    registry->free_head = slot->next_free;
    slot->next_free = HTH_ENTITY_FREE_LIST_END;
    slot->state = HTH_ENTITY_SLOT_ALIVE;
    registry->live_count++;
    out_handle->index = index;
    out_handle->generation = slot->generation;
    return true;
}

bool hth_entity_registry_destroy_entity(HTHEntityRegistry *registry,
                                        HTHEntityHandle handle)
{
    HTHEntitySlot *slot;

    if (!hth_entity_registry_is_alive(registry, handle)) {
        return false;
    }
    slot = &registry->slots[handle.index];
    registry->live_count--;
    if (slot->generation == UINT32_MAX) {
        slot->state = HTH_ENTITY_SLOT_RETIRED;
        slot->next_free = HTH_ENTITY_FREE_LIST_END;
    } else {
        slot->generation++;
        slot->state = HTH_ENTITY_SLOT_FREE;
        slot->next_free = registry->free_head;
        registry->free_head = handle.index;
    }
    return true;
}

bool hth_entity_registry_is_alive(const HTHEntityRegistry *registry,
                                  HTHEntityHandle handle)
{
    const HTHEntitySlot *slot;

    if (registry == NULL || handle.index == UINT32_MAX ||
        handle.generation == 0U || (size_t)handle.index >= registry->capacity) {
        return false;
    }
    slot = &registry->slots[handle.index];
    return slot->state == HTH_ENTITY_SLOT_ALIVE &&
           slot->generation == handle.generation;
}

size_t hth_entity_registry_live_count(const HTHEntityRegistry *registry)
{
    return registry != NULL ? registry->live_count : 0U;
}

void hth_entity_iterator_begin(HTHEntityIterator *iterator)
{
    if (iterator != NULL) {
        iterator->next_index = 0U;
    }
}

bool hth_entity_iterator_next(const HTHEntityRegistry *registry,
                              HTHEntityIterator *iterator,
                              HTHEntityHandle *out_handle)
{
    if (out_handle == NULL) {
        return false;
    }
    *out_handle = hth_entity_handle_invalid();
    if (registry == NULL || iterator == NULL) {
        return false;
    }
    while (iterator->next_index < registry->capacity) {
        size_t index = iterator->next_index++;
        const HTHEntitySlot *slot = &registry->slots[index];

        if (slot->state == HTH_ENTITY_SLOT_ALIVE) {
            out_handle->index = (uint32_t)index;
            out_handle->generation = slot->generation;
            return true;
        }
    }
    return false;
}

#if defined(HTH_ENTITY_TESTING)
bool hth_entity_registry_test_set_generation(
    HTHEntityRegistry *registry, HTHEntityHandle handle,
    uint32_t generation, HTHEntityHandle *out_handle)
{
    if (out_handle == NULL) {
        return false;
    }
    *out_handle = hth_entity_handle_invalid();
    if (generation == 0U || !hth_entity_registry_is_alive(registry, handle)) {
        return false;
    }
    registry->slots[handle.index].generation = generation;
    out_handle->index = handle.index;
    out_handle->generation = generation;
    return true;
}

bool hth_entity_registry_test_validate(const HTHEntityRegistry *registry)
{
    size_t alive_count = 0U;
    size_t free_count = 0U;
    size_t retired_count = 0U;
    size_t visited_count = 0U;
    size_t index;
    uint32_t free_index;

    if (registry == NULL || registry->slots == NULL ||
        registry->capacity == 0U || registry->capacity > (size_t)UINT32_MAX ||
        registry->live_count > registry->capacity) {
        return false;
    }
    for (index = 0U; index < registry->capacity; ++index) {
        const HTHEntitySlot *slot = &registry->slots[index];

        if (slot->generation == 0U) {
            return false;
        }
        switch (slot->state) {
        case HTH_ENTITY_SLOT_ALIVE:
            alive_count++;
            if (slot->next_free != HTH_ENTITY_FREE_LIST_END) {
                return false;
            }
            break;
        case HTH_ENTITY_SLOT_FREE:
            free_count++;
            break;
        case HTH_ENTITY_SLOT_RETIRED:
            retired_count++;
            if (slot->next_free != HTH_ENTITY_FREE_LIST_END) {
                return false;
            }
            break;
        default:
            return false;
        }
    }
    free_index = registry->free_head;
    while (free_index != HTH_ENTITY_FREE_LIST_END) {
        const HTHEntitySlot *slot;

        if ((size_t)free_index >= registry->capacity ||
            visited_count >= registry->capacity) {
            return false;
        }
        slot = &registry->slots[free_index];
        if (slot->state != HTH_ENTITY_SLOT_FREE) {
            return false;
        }
        visited_count++;
        free_index = slot->next_free;
    }
    return alive_count == registry->live_count &&
           visited_count == free_count &&
           alive_count + free_count + retired_count == registry->capacity;
}
#endif
