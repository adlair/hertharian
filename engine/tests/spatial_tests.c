#include "spatial.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,       \
                    __LINE__, #condition);                                   \
            return false;                                                    \
        }                                                                    \
    } while (0)

static HTHSpatialTransform transform(float x, float y, float z, float yaw)
{
    HTHSpatialTransform value = {{x, y, z}, yaw};

    return value;
}

static bool transform_equal(HTHSpatialTransform left,
                            HTHSpatialTransform right)
{
    return left.position.x == right.position.x &&
           left.position.y == right.position.y &&
           left.position.z == right.position.z && left.yaw == right.yaw;
}

static bool transform_is_zero(HTHSpatialTransform value)
{
    return transform_equal(value, transform(0.0F, 0.0F, 0.0F, 0.0F));
}

static bool test_store_lifecycle_and_invalid_arguments(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *store = hth_spatial_store_create();
    HTHEntityHandle entity;
    HTHEntityHandle output_entity = {7U, 9U};
    HTHSpatialTransform value = transform(1.0F, 2.0F, 3.0F, 0.5F);
    HTHSpatialTransform output = value;
    HTHSpatialIterator iterator = {5U};

    CHECK(entities != NULL && store != NULL);
    CHECK(hth_entity_registry_create_entity(entities, &entity));
    CHECK(!hth_spatial_store_attach(NULL, entities, entity, &value));
    CHECK(!hth_spatial_store_attach(store, NULL, entity, &value));
    CHECK(!hth_spatial_store_attach(store, entities, entity, NULL));
    CHECK(!hth_spatial_store_has(NULL, entities, entity));
    CHECK(!hth_spatial_store_has(store, NULL, entity));
    CHECK(!hth_spatial_store_get(NULL, entities, entity, &output));
    CHECK(transform_is_zero(output));
    output = value;
    CHECK(!hth_spatial_store_get(store, NULL, entity, &output));
    CHECK(transform_is_zero(output));
    CHECK(!hth_spatial_store_get(store, entities, entity, NULL));
    CHECK(!hth_spatial_store_set(NULL, entities, entity, &value));
    CHECK(!hth_spatial_store_set(store, NULL, entity, &value));
    CHECK(!hth_spatial_store_set(store, entities, entity, NULL));
    CHECK(!hth_spatial_store_remove(NULL, entities, entity));
    CHECK(!hth_spatial_store_remove(store, NULL, entity));
    hth_spatial_iterator_begin(NULL);
    CHECK(!hth_spatial_iterator_next(
        NULL, entities, &iterator, &output_entity, &output));
    CHECK(hth_entity_handle_equal(output_entity,
                                  hth_entity_handle_invalid()));
    CHECK(transform_is_zero(output));
    output_entity = entity;
    output = value;
    CHECK(!hth_spatial_iterator_next(
        store, NULL, &iterator, &output_entity, &output));
    CHECK(hth_entity_handle_equal(output_entity,
                                  hth_entity_handle_invalid()));
    CHECK(transform_is_zero(output));
    CHECK(!hth_spatial_iterator_next(
        store, entities, NULL, &output_entity, &output));
    output = value;
    CHECK(!hth_spatial_iterator_next(
        store, entities, &iterator, NULL, &output));
    CHECK(transform_is_zero(output));
    output_entity = entity;
    CHECK(!hth_spatial_iterator_next(
        store, entities, &iterator, &output_entity, NULL));
    CHECK(hth_entity_handle_equal(output_entity,
                                  hth_entity_handle_invalid()));
    hth_spatial_store_destroy(store);
    hth_spatial_store_destroy(NULL);
    CHECK(hth_entity_registry_is_alive(entities, entity));
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_attach_get_set_remove(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *store = hth_spatial_store_create();
    HTHEntityHandle entity;
    HTHSpatialTransform first = transform(1.0F, 2.0F, 3.0F, 0.5F);
    HTHSpatialTransform second = transform(-4.0F, 8.0F, 12.0F, -2.0F);
    HTHSpatialTransform output = second;

    CHECK(entities != NULL && store != NULL);
    CHECK(hth_entity_registry_create_entity(entities, &entity));
    CHECK(entity.index == 0U && entity.generation == 1U);
    CHECK(!hth_spatial_store_has(store, entities, entity));
    CHECK(!hth_spatial_store_get(store, entities, entity, &output));
    CHECK(transform_is_zero(output));
    CHECK(!hth_spatial_store_set(store, entities, entity, &first));
    CHECK(!hth_spatial_store_remove(store, entities, entity));

    CHECK(hth_spatial_store_attach(store, entities, entity, &first));
    CHECK(hth_spatial_store_has(store, entities, entity));
    CHECK(hth_spatial_store_get(store, entities, entity, &output));
    CHECK(transform_equal(output, first));
    CHECK(!hth_spatial_store_attach(store, entities, entity, &second));
    CHECK(hth_spatial_store_get(store, entities, entity, &output));
    CHECK(transform_equal(output, first));
    CHECK(hth_spatial_store_set(store, entities, entity, &second));
    CHECK(hth_spatial_store_get(store, entities, entity, &output));
    CHECK(transform_equal(output, second));
    CHECK(hth_spatial_store_remove(store, entities, entity));
    CHECK(hth_entity_registry_is_alive(entities, entity));
    CHECK(!hth_spatial_store_has(store, entities, entity));
    output = second;
    CHECK(!hth_spatial_store_get(store, entities, entity, &output));
    CHECK(transform_is_zero(output));
    CHECK(!hth_spatial_store_remove(store, entities, entity));
    CHECK(hth_entity_registry_is_alive(entities, entity));
    hth_spatial_store_destroy(store);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_finite_validation_and_yaw_roundtrip(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *store = hth_spatial_store_create();
    HTHEntityHandle entity;
    HTHSpatialTransform valid = transform(2.0F, 3.0F, 4.0F, 21.991148F);
    HTHSpatialTransform invalid[] = {
        {{NAN, 0.0F, 0.0F}, 0.0F},
        {{INFINITY, 0.0F, 0.0F}, 0.0F},
        {{-INFINITY, 0.0F, 0.0F}, 0.0F},
        {{0.0F, NAN, 0.0F}, 0.0F},
        {{0.0F, INFINITY, 0.0F}, 0.0F},
        {{0.0F, -INFINITY, 0.0F}, 0.0F},
        {{0.0F, 0.0F, NAN}, 0.0F},
        {{0.0F, 0.0F, INFINITY}, 0.0F},
        {{0.0F, 0.0F, -INFINITY}, 0.0F},
        {{0.0F, 0.0F, 0.0F}, NAN},
        {{0.0F, 0.0F, 0.0F}, INFINITY},
        {{0.0F, 0.0F, 0.0F}, -INFINITY}
    };
    HTHSpatialTransform output;
    size_t index;

    CHECK(entities != NULL && store != NULL);
    CHECK(hth_entity_registry_create_entity(entities, &entity));
    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        CHECK(!hth_spatial_store_attach(
            store, entities, entity, &invalid[index]));
        CHECK(!hth_spatial_store_has(store, entities, entity));
    }
    CHECK(hth_spatial_store_attach(store, entities, entity, &valid));
    CHECK(hth_spatial_store_get(store, entities, entity, &output));
    CHECK(transform_equal(output, valid));
    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        CHECK(!hth_spatial_store_set(
            store, entities, entity, &invalid[index]));
        CHECK(hth_spatial_store_get(store, entities, entity, &output));
        CHECK(transform_equal(output, valid));
    }
    hth_spatial_store_destroy(store);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_dead_and_reused_entity_isolation(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *store = hth_spatial_store_create();
    HTHEntityHandle stale;
    HTHEntityHandle current;
    HTHSpatialTransform old_transform =
        transform(1.0F, 2.0F, 3.0F, 0.25F);
    HTHSpatialTransform new_transform =
        transform(9.0F, 8.0F, 7.0F, -3.5F);
    HTHSpatialTransform attempted =
        transform(100.0F, 200.0F, 300.0F, 5.0F);
    HTHSpatialTransform output = old_transform;

    CHECK(entities != NULL && store != NULL);
    CHECK(hth_entity_registry_create_entity(entities, &stale));
    CHECK(hth_spatial_store_attach(store, entities, stale, &old_transform));
    CHECK(hth_entity_registry_destroy_entity(entities, stale));
    CHECK(!hth_spatial_store_has(store, entities, stale));
    CHECK(!hth_spatial_store_get(store, entities, stale, &output));
    CHECK(transform_is_zero(output));
    CHECK(!hth_spatial_store_attach(store, entities, stale, &attempted));

    CHECK(hth_entity_registry_create_entity(entities, &current));
    CHECK(current.index == stale.index);
    CHECK(current.generation == stale.generation + 1U);
    CHECK(!hth_spatial_store_has(store, entities, current));
    output = old_transform;
    CHECK(!hth_spatial_store_get(store, entities, current, &output));
    CHECK(transform_is_zero(output));
    CHECK(!hth_spatial_store_set(store, entities, current, &attempted));
    CHECK(!hth_spatial_store_remove(store, entities, current));

    CHECK(hth_spatial_store_attach(
        store, entities, current, &new_transform));
    CHECK(hth_spatial_store_get(store, entities, current, &output));
    CHECK(transform_equal(output, new_transform));
    CHECK(!hth_spatial_store_get(store, entities, stale, &output));
    CHECK(transform_is_zero(output));
    CHECK(!hth_spatial_store_set(store, entities, stale, &attempted));
    CHECK(!hth_spatial_store_remove(store, entities, stale));
    CHECK(hth_spatial_store_get(store, entities, current, &output));
    CHECK(transform_equal(output, new_transform));
    hth_spatial_store_destroy(store);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_growth_sparse_associations_and_copy_out(void)
{
    enum { ENTITY_COUNT = 130 };
    static const uint32_t attached_indices[] = {1U, 7U, 65U, 129U};
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *store = hth_spatial_store_create();
    HTHEntityHandle handles[ENTITY_COUNT];
    HTHSpatialTransform expected[4];
    HTHSpatialTransform output;
    HTHSpatialIterator iterator;
    HTHEntityHandle iterated;
    size_t index;

    CHECK(entities != NULL && store != NULL);
    for (index = 0U; index < ENTITY_COUNT; ++index) {
        CHECK(hth_entity_registry_create_entity(entities, &handles[index]));
    }
    for (index = 0U; index < sizeof(attached_indices) /
                               sizeof(attached_indices[0]); ++index) {
        uint32_t entity_index = attached_indices[index];

        expected[index] = transform(
            (float)entity_index, (float)index + 0.5F,
            -(float)entity_index, (float)index - 1.25F);
        CHECK(hth_spatial_store_attach(
            store, entities, handles[entity_index], &expected[index]));
    }
    for (index = 0U; index < ENTITY_COUNT; ++index) {
        bool expected_present = index == 1U || index == 7U ||
                                index == 65U || index == 129U;

        CHECK(hth_spatial_store_has(store, entities, handles[index]) ==
              expected_present);
    }
    for (index = 0U; index < sizeof(attached_indices) /
                               sizeof(attached_indices[0]); ++index) {
        CHECK(hth_spatial_store_get(
            store, entities, handles[attached_indices[index]], &output));
        CHECK(transform_equal(output, expected[index]));
    }
    hth_spatial_iterator_begin(&iterator);
    for (index = 0U; index < sizeof(attached_indices) /
                               sizeof(attached_indices[0]); ++index) {
        CHECK(hth_spatial_iterator_next(
            store, entities, &iterator, &iterated, &output));
        CHECK(hth_entity_handle_equal(
            iterated, handles[attached_indices[index]]));
        CHECK(transform_equal(output, expected[index]));
        CHECK(hth_entity_registry_is_alive(entities, iterated));
    }
    CHECK(!hth_spatial_iterator_next(
        store, entities, &iterator, &iterated, &output));
    CHECK(hth_entity_handle_equal(iterated, hth_entity_handle_invalid()));
    CHECK(transform_is_zero(output));
    CHECK(!hth_spatial_iterator_next(
        store, entities, &iterator, &iterated, &output));
    hth_spatial_store_destroy(store);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_iteration_removed_dead_and_reused_entries(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *store = hth_spatial_store_create();
    HTHEntityHandle handles[6];
    HTHEntityHandle stale;
    HTHEntityHandle replacement;
    HTHEntityHandle iterated;
    HTHSpatialTransform values[6];
    HTHSpatialTransform replacement_value =
        transform(50.0F, 51.0F, 52.0F, 5.0F);
    HTHSpatialTransform output;
    HTHSpatialIterator iterator;
    size_t index;
    size_t yielded;

    CHECK(entities != NULL && store != NULL);
    hth_spatial_iterator_begin(&iterator);
    CHECK(!hth_spatial_iterator_next(
        store, entities, &iterator, &iterated, &output));
    for (index = 0U; index < 6U; ++index) {
        values[index] = transform((float)index, 1.0F, 2.0F, (float)index);
        CHECK(hth_entity_registry_create_entity(entities, &handles[index]));
        CHECK(hth_spatial_store_attach(
            store, entities, handles[index], &values[index]));
    }
    CHECK(hth_spatial_store_remove(store, entities, handles[2]));
    stale = handles[4];
    CHECK(hth_entity_registry_destroy_entity(entities, stale));

    hth_spatial_iterator_begin(&iterator);
    yielded = 0U;
    while (hth_spatial_iterator_next(
               store, entities, &iterator, &iterated, &output)) {
        static const uint32_t expected_indices[] = {0U, 1U, 3U, 5U};

        CHECK(yielded < sizeof(expected_indices) /
                            sizeof(expected_indices[0]));
        CHECK(iterated.index == expected_indices[yielded]);
        CHECK(transform_equal(output, values[iterated.index]));
        CHECK(hth_entity_registry_is_alive(entities, iterated));
        yielded++;
    }
    CHECK(yielded == 4U);

    CHECK(hth_entity_registry_create_entity(entities, &replacement));
    CHECK(replacement.index == stale.index);
    hth_spatial_iterator_begin(&iterator);
    yielded = 0U;
    while (hth_spatial_iterator_next(
               store, entities, &iterator, &iterated, &output)) {
        CHECK(iterated.index != replacement.index);
        yielded++;
    }
    CHECK(yielded == 4U);

    CHECK(hth_spatial_store_attach(
        store, entities, replacement, &replacement_value));
    hth_spatial_iterator_begin(&iterator);
    yielded = 0U;
    while (hth_spatial_iterator_next(
               store, entities, &iterator, &iterated, &output)) {
        if (iterated.index == replacement.index) {
            CHECK(hth_entity_handle_equal(iterated, replacement));
            CHECK(transform_equal(output, replacement_value));
        }
        CHECK(!hth_entity_handle_equal(iterated, stale));
        yielded++;
    }
    CHECK(yielded == 5U);
    hth_spatial_store_destroy(store);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_multiple_pairs_and_store_destroy_with_entries(void)
{
    HTHEntityRegistry *entities_a = hth_entity_registry_create();
    HTHEntityRegistry *entities_b = hth_entity_registry_create();
    HTHSpatialStore *store_a = hth_spatial_store_create();
    HTHSpatialStore *store_b = hth_spatial_store_create();
    HTHEntityHandle entity_a;
    HTHEntityHandle entity_b;
    HTHSpatialTransform transform_a = transform(1.0F, 0.0F, 0.0F, 1.0F);
    HTHSpatialTransform transform_b = transform(2.0F, 0.0F, 0.0F, 2.0F);
    HTHSpatialTransform output;
    HTHEntityHandle extra;
    size_t index;

    CHECK(entities_a != NULL && entities_b != NULL &&
          store_a != NULL && store_b != NULL);
    CHECK(hth_entity_registry_create_entity(entities_a, &entity_a));
    CHECK(hth_entity_registry_create_entity(entities_b, &entity_b));
    CHECK(hth_entity_handle_equal(entity_a, entity_b));
    CHECK(hth_spatial_store_attach(
        store_a, entities_a, entity_a, &transform_a));
    CHECK(hth_spatial_store_attach(
        store_b, entities_b, entity_b, &transform_b));
    CHECK(hth_spatial_store_get(store_a, entities_a, entity_a, &output));
    CHECK(transform_equal(output, transform_a));
    CHECK(hth_spatial_store_get(store_b, entities_b, entity_b, &output));
    CHECK(transform_equal(output, transform_b));

    for (index = 1U; index < 100U; ++index) {
        CHECK(hth_entity_registry_create_entity(entities_a, &extra));
        CHECK(hth_spatial_store_attach(
            store_a, entities_a, extra, &transform_a));
    }
    hth_spatial_store_destroy(store_a);
    CHECK(hth_entity_registry_is_alive(entities_a, entity_a));
    CHECK(hth_entity_registry_live_count(entities_a) == 100U);
    hth_spatial_store_destroy(store_b);
    CHECK(hth_entity_registry_is_alive(entities_b, entity_b));
    hth_entity_registry_destroy(entities_a);
    hth_entity_registry_destroy(entities_b);
    return true;
}

int main(void)
{
    if (!test_store_lifecycle_and_invalid_arguments() ||
        !test_attach_get_set_remove() ||
        !test_finite_validation_and_yaw_roundtrip() ||
        !test_dead_and_reused_entity_isolation() ||
        !test_growth_sparse_associations_and_copy_out() ||
        !test_iteration_removed_dead_and_reused_entries() ||
        !test_multiple_pairs_and_store_destroy_with_entries()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
