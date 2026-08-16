#ifndef HTH_ENTITY_H
#define HTH_ENTITY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t index;
    uint32_t generation;
} HTHEntityHandle;

typedef struct HTHEntityRegistry HTHEntityRegistry;

typedef struct {
    size_t next_index;
} HTHEntityIterator;

HTHEntityHandle hth_entity_handle_invalid(void);
bool hth_entity_handle_equal(HTHEntityHandle left, HTHEntityHandle right);

HTHEntityRegistry *hth_entity_registry_create(void);
void hth_entity_registry_destroy(HTHEntityRegistry *registry);
bool hth_entity_registry_create_entity(HTHEntityRegistry *registry,
                                       HTHEntityHandle *out_handle);
bool hth_entity_registry_destroy_entity(HTHEntityRegistry *registry,
                                        HTHEntityHandle handle);
bool hth_entity_registry_is_alive(const HTHEntityRegistry *registry,
                                  HTHEntityHandle handle);
size_t hth_entity_registry_live_count(const HTHEntityRegistry *registry);

void hth_entity_iterator_begin(HTHEntityIterator *iterator);
bool hth_entity_iterator_next(const HTHEntityRegistry *registry,
                              HTHEntityIterator *iterator,
                              HTHEntityHandle *out_handle);

#if defined(HTH_ENTITY_TESTING)
bool hth_entity_registry_test_set_generation(
    HTHEntityRegistry *registry, HTHEntityHandle handle,
    uint32_t generation, HTHEntityHandle *out_handle);
bool hth_entity_registry_test_validate(const HTHEntityRegistry *registry);
#endif

#endif
