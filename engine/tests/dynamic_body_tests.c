#include "dynamic_body.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,      \
                    __LINE__, #condition);                                   \
            return false;                                                    \
        }                                                                    \
    } while (0)

static HTHSpatialTransform transform(float x, float y, float z, float yaw)
{
    HTHSpatialTransform value = {{x, y, z}, yaw};

    return value;
}

static HTHDynamicBody body(float hx, float hy, float hz,
                           float vx, float vy, float vz)
{
    HTHDynamicBody value = {{hx, hy, hz}, {vx, vy, vz}};

    return value;
}

static bool vector_equal(HTHVec3 left, HTHVec3 right)
{
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

static bool body_equal(HTHDynamicBody left, HTHDynamicBody right)
{
    return vector_equal(left.half_extents, right.half_extents) &&
           vector_equal(left.velocity, right.velocity);
}

static bool body_is_zero(HTHDynamicBody value)
{
    return body_equal(value, body(0.0F, 0.0F, 0.0F,
                                  0.0F, 0.0F, 0.0F));
}

static bool attach_spatial(HTHSpatialStore *spatial,
                           HTHEntityRegistry *entities,
                           HTHEntityHandle entity,
                           HTHSpatialTransform value)
{
    return hth_spatial_store_attach(spatial, entities, entity, &value);
}

static bool test_lifecycle_attach_and_invalid_arguments(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *spatial = hth_spatial_store_create();
    HTHDynamicBodyStore *bodies = hth_dynamic_body_store_create();
    HTHEntityHandle entity;
    HTHEntityHandle without_spatial;
    HTHDynamicBody value = body(0.5F, 1.0F, 0.75F,
                                1.0F, -2.0F, 3.0F);
    HTHDynamicBody output = value;

    CHECK(entities != NULL && spatial != NULL && bodies != NULL);
    CHECK(hth_entity_registry_create_entity(entities, &entity));
    CHECK(hth_entity_registry_create_entity(entities, &without_spatial));
    CHECK(attach_spatial(spatial, entities, entity,
                         transform(1.0F, 2.0F, 3.0F, 0.25F)));
    CHECK(!hth_dynamic_body_attach(NULL, entities, spatial, entity, &value));
    CHECK(!hth_dynamic_body_attach(bodies, NULL, spatial, entity, &value));
    CHECK(!hth_dynamic_body_attach(bodies, entities, NULL, entity, &value));
    CHECK(!hth_dynamic_body_attach(bodies, entities, spatial, entity, NULL));
    CHECK(!hth_dynamic_body_attach(
        bodies, entities, spatial, without_spatial, &value));
    CHECK(!hth_dynamic_body_has(bodies, entities, entity));
    CHECK(!hth_dynamic_body_get(bodies, entities, entity, &output));
    CHECK(body_is_zero(output));
    CHECK(!hth_dynamic_body_get(bodies, entities, entity, NULL));

    CHECK(hth_dynamic_body_attach(
        bodies, entities, spatial, entity, &value));
    CHECK(hth_dynamic_body_has(bodies, entities, entity));
    CHECK(hth_dynamic_body_get(bodies, entities, entity, &output));
    CHECK(body_equal(output, value));
    CHECK(!hth_dynamic_body_attach(
        bodies, entities, spatial, entity,
        &(HTHDynamicBody){{2.0F, 2.0F, 2.0F}, {9.0F, 9.0F, 9.0F}}));
    CHECK(hth_dynamic_body_get(bodies, entities, entity, &output));
    CHECK(body_equal(output, value));

    hth_dynamic_body_store_destroy(bodies);
    hth_dynamic_body_store_destroy(NULL);
    CHECK(hth_entity_registry_is_alive(entities, entity));
    CHECK(hth_spatial_store_has(spatial, entities, entity));
    hth_spatial_store_destroy(spatial);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_non_finite_and_non_positive_validation(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *spatial = hth_spatial_store_create();
    HTHDynamicBodyStore *bodies = hth_dynamic_body_store_create();
    HTHEntityHandle entity;
    HTHDynamicBody invalid_extents[] = {
        {{0.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}},
        {{1.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 0.0F}},
        {{1.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 0.0F}},
        {{-1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}},
        {{1.0F, -1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}},
        {{1.0F, 1.0F, -1.0F}, {0.0F, 0.0F, 0.0F}},
        {{NAN, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}},
        {{INFINITY, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}},
        {{-INFINITY, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}},
        {{1.0F, NAN, 1.0F}, {0.0F, 0.0F, 0.0F}},
        {{1.0F, INFINITY, 1.0F}, {0.0F, 0.0F, 0.0F}},
        {{1.0F, -INFINITY, 1.0F}, {0.0F, 0.0F, 0.0F}},
        {{1.0F, 1.0F, NAN}, {0.0F, 0.0F, 0.0F}},
        {{1.0F, 1.0F, INFINITY}, {0.0F, 0.0F, 0.0F}},
        {{1.0F, 1.0F, -INFINITY}, {0.0F, 0.0F, 0.0F}}
    };
    HTHVec3 invalid_velocity[] = {
        {NAN, 0.0F, 0.0F}, {INFINITY, 0.0F, 0.0F},
        {-INFINITY, 0.0F, 0.0F}, {0.0F, NAN, 0.0F},
        {0.0F, INFINITY, 0.0F}, {0.0F, -INFINITY, 0.0F},
        {0.0F, 0.0F, NAN}, {0.0F, 0.0F, INFINITY},
        {0.0F, 0.0F, -INFINITY}
    };
    HTHDynamicBody valid = body(1.0F, 2.0F, 3.0F,
                                4.0F, 5.0F, 6.0F);
    HTHDynamicBody output;
    size_t index;

    CHECK(entities != NULL && spatial != NULL && bodies != NULL);
    CHECK(hth_entity_registry_create_entity(entities, &entity));
    CHECK(attach_spatial(spatial, entities, entity,
                         transform(0.0F, 0.0F, 0.0F, 0.0F)));
    for (index = 0U; index < sizeof(invalid_extents) /
                               sizeof(invalid_extents[0]); ++index) {
        CHECK(!hth_dynamic_body_attach(
            bodies, entities, spatial, entity, &invalid_extents[index]));
    }
    for (index = 0U; index < sizeof(invalid_velocity) /
                               sizeof(invalid_velocity[0]); ++index) {
        HTHDynamicBody invalid = valid;

        invalid.velocity = invalid_velocity[index];
        CHECK(!hth_dynamic_body_attach(
            bodies, entities, spatial, entity, &invalid));
    }
    CHECK(hth_dynamic_body_attach(
        bodies, entities, spatial, entity, &valid));
    for (index = 0U; index < sizeof(invalid_velocity) /
                               sizeof(invalid_velocity[0]); ++index) {
        CHECK(!hth_dynamic_body_set_velocity(
            bodies, entities, entity, invalid_velocity[index]));
        CHECK(hth_dynamic_body_get(bodies, entities, entity, &output));
        CHECK(body_equal(output, valid));
    }
    hth_dynamic_body_store_destroy(bodies);
    hth_spatial_store_destroy(spatial);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_velocity_remove_and_spatial_independence(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *spatial = hth_spatial_store_create();
    HTHDynamicBodyStore *bodies = hth_dynamic_body_store_create();
    HTHEntityHandle entity;
    HTHDynamicBody original = body(0.5F, 0.75F, 1.0F,
                                   1.0F, 2.0F, 3.0F);
    HTHDynamicBody output;
    HTHVec3 changed = {-4.0F, 5.0F, -6.0F};

    CHECK(entities != NULL && spatial != NULL && bodies != NULL);
    CHECK(hth_entity_registry_create_entity(entities, &entity));
    CHECK(attach_spatial(spatial, entities, entity,
                         transform(2.0F, 3.0F, 4.0F, 1.0F)));
    CHECK(hth_dynamic_body_attach(
        bodies, entities, spatial, entity, &original));
    CHECK(hth_dynamic_body_set_velocity(bodies, entities, entity, changed));
    CHECK(hth_dynamic_body_get(bodies, entities, entity, &output));
    CHECK(vector_equal(output.half_extents, original.half_extents));
    CHECK(vector_equal(output.velocity, changed));

    CHECK(hth_spatial_store_remove(spatial, entities, entity));
    CHECK(hth_dynamic_body_has(bodies, entities, entity));
    CHECK(hth_dynamic_body_get(bodies, entities, entity, &output));
    CHECK(vector_equal(output.velocity, changed));
    CHECK(attach_spatial(spatial, entities, entity,
                         transform(10.0F, 20.0F, 30.0F, -2.0F)));
    CHECK(hth_dynamic_body_get(bodies, entities, entity, &output));
    CHECK(vector_equal(output.velocity, changed));

    CHECK(hth_dynamic_body_remove(bodies, entities, entity));
    CHECK(!hth_dynamic_body_has(bodies, entities, entity));
    CHECK(hth_entity_registry_is_alive(entities, entity));
    CHECK(hth_spatial_store_has(spatial, entities, entity));
    CHECK(!hth_dynamic_body_remove(bodies, entities, entity));
    hth_dynamic_body_store_destroy(bodies);
    hth_spatial_store_destroy(spatial);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_dead_stale_and_reused_entity_isolation(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *spatial = hth_spatial_store_create();
    HTHDynamicBodyStore *bodies = hth_dynamic_body_store_create();
    HTHEntityHandle stale;
    HTHEntityHandle current;
    HTHDynamicBody old_body = body(1.0F, 1.0F, 1.0F,
                                   1.0F, 2.0F, 3.0F);
    HTHDynamicBody new_body = body(2.0F, 2.0F, 2.0F,
                                   4.0F, 5.0F, 6.0F);
    HTHDynamicBody output = old_body;
    HTHVec3 attempted = {99.0F, 98.0F, 97.0F};

    CHECK(entities != NULL && spatial != NULL && bodies != NULL);
    CHECK(hth_entity_registry_create_entity(entities, &stale));
    CHECK(attach_spatial(spatial, entities, stale,
                         transform(1.0F, 2.0F, 3.0F, 0.0F)));
    CHECK(hth_dynamic_body_attach(
        bodies, entities, spatial, stale, &old_body));
    CHECK(hth_entity_registry_destroy_entity(entities, stale));
    CHECK(!hth_dynamic_body_has(bodies, entities, stale));
    CHECK(!hth_dynamic_body_get(bodies, entities, stale, &output));
    CHECK(body_is_zero(output));
    CHECK(!hth_dynamic_body_attach(
        bodies, entities, spatial, stale, &new_body));

    CHECK(hth_entity_registry_create_entity(entities, &current));
    CHECK(current.index == stale.index);
    CHECK(current.generation == stale.generation + 1U);
    CHECK(!hth_dynamic_body_has(bodies, entities, current));
    CHECK(!hth_dynamic_body_get(bodies, entities, current, &output));
    CHECK(body_is_zero(output));
    CHECK(!hth_dynamic_body_set_velocity(
        bodies, entities, current, attempted));
    CHECK(!hth_dynamic_body_remove(bodies, entities, current));

    CHECK(attach_spatial(spatial, entities, current,
                         transform(9.0F, 8.0F, 7.0F, 0.0F)));
    CHECK(!hth_dynamic_body_attach(
        bodies, entities, spatial, stale, &old_body));
    CHECK(hth_dynamic_body_attach(
        bodies, entities, spatial, current, &new_body));
    CHECK(!hth_dynamic_body_get(bodies, entities, stale, &output));
    CHECK(body_is_zero(output));
    CHECK(!hth_dynamic_body_set_velocity(
        bodies, entities, stale, attempted));
    CHECK(!hth_dynamic_body_remove(bodies, entities, stale));
    CHECK(hth_dynamic_body_get(bodies, entities, current, &output));
    CHECK(body_equal(output, new_body));
    hth_dynamic_body_store_destroy(bodies);
    hth_spatial_store_destroy(spatial);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_growth_sparse_iteration_and_spatial_independence(void)
{
    enum { ENTITY_COUNT = 130 };
    static const uint32_t attached_indices[] = {1U, 7U, 65U, 129U};
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *spatial = hth_spatial_store_create();
    HTHDynamicBodyStore *bodies = hth_dynamic_body_store_create();
    HTHEntityHandle handles[ENTITY_COUNT];
    HTHDynamicBody expected[4];
    HTHDynamicBody output;
    HTHDynamicBodyIterator iterator;
    HTHEntityHandle iterated;
    HTHEntityHandle replacement;
    size_t index;

    CHECK(entities != NULL && spatial != NULL && bodies != NULL);
    hth_dynamic_body_iterator_begin(&iterator);
    CHECK(!hth_dynamic_body_iterator_next(
        bodies, entities, &iterator, &iterated, &output));
    for (index = 0U; index < ENTITY_COUNT; ++index) {
        CHECK(hth_entity_registry_create_entity(entities, &handles[index]));
    }
    for (index = 0U; index < sizeof(attached_indices) /
                               sizeof(attached_indices[0]); ++index) {
        uint32_t entity_index = attached_indices[index];

        expected[index] = body(
            (float)index + 0.5F, 1.0F, 1.5F,
            (float)entity_index, -(float)index, (float)index + 2.0F);
        CHECK(attach_spatial(
            spatial, entities, handles[entity_index],
            transform((float)entity_index, 0.0F, 0.0F, 0.0F)));
        CHECK(hth_dynamic_body_attach(
            bodies, entities, spatial, handles[entity_index],
            &expected[index]));
    }
    CHECK(hth_spatial_store_remove(
        spatial, entities, handles[attached_indices[1]]));
    CHECK(hth_dynamic_body_has(
        bodies, entities, handles[attached_indices[1]]));

    hth_dynamic_body_iterator_begin(&iterator);
    for (index = 0U; index < sizeof(attached_indices) /
                               sizeof(attached_indices[0]); ++index) {
        CHECK(hth_dynamic_body_iterator_next(
            bodies, entities, &iterator, &iterated, &output));
        CHECK(hth_entity_handle_equal(
            iterated, handles[attached_indices[index]]));
        CHECK(body_equal(output, expected[index]));
    }
    CHECK(!hth_dynamic_body_iterator_next(
        bodies, entities, &iterator, &iterated, &output));
    CHECK(hth_entity_handle_equal(iterated, hth_entity_handle_invalid()));
    CHECK(body_is_zero(output));
    CHECK(!hth_dynamic_body_iterator_next(
        bodies, entities, &iterator, &iterated, &output));

    CHECK(hth_dynamic_body_remove(
        bodies, entities, handles[attached_indices[0]]));
    CHECK(hth_entity_registry_destroy_entity(
        entities, handles[attached_indices[2]]));
    hth_dynamic_body_iterator_begin(&iterator);
    CHECK(hth_dynamic_body_iterator_next(
        bodies, entities, &iterator, &iterated, &output));
    CHECK(iterated.index == 7U);
    CHECK(hth_dynamic_body_iterator_next(
        bodies, entities, &iterator, &iterated, &output));
    CHECK(iterated.index == 129U);
    CHECK(!hth_dynamic_body_iterator_next(
        bodies, entities, &iterator, &iterated, &output));

    CHECK(hth_entity_registry_create_entity(entities, &replacement));
    CHECK(replacement.index == 65U);
    CHECK(attach_spatial(spatial, entities, replacement,
                         transform(65.0F, 1.0F, 0.0F, 0.0F)));
    CHECK(hth_dynamic_body_attach(
        bodies, entities, spatial, replacement, &expected[2]));
    hth_dynamic_body_iterator_begin(&iterator);
    CHECK(hth_dynamic_body_iterator_next(
        bodies, entities, &iterator, &iterated, &output));
    CHECK(iterated.index == 7U);
    CHECK(hth_dynamic_body_iterator_next(
        bodies, entities, &iterator, &iterated, &output));
    CHECK(hth_entity_handle_equal(iterated, replacement));
    CHECK(body_equal(output, expected[2]));
    CHECK(hth_dynamic_body_iterator_next(
        bodies, entities, &iterator, &iterated, &output));
    CHECK(iterated.index == 129U);
    CHECK(!hth_dynamic_body_iterator_next(
        bodies, entities, &iterator, &iterated, &output));
    hth_dynamic_body_store_destroy(bodies);
    hth_spatial_store_destroy(spatial);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_multiple_pairs_and_destroy_with_present_bodies(void)
{
    HTHEntityRegistry *entities_a = hth_entity_registry_create();
    HTHEntityRegistry *entities_b = hth_entity_registry_create();
    HTHSpatialStore *spatial_a = hth_spatial_store_create();
    HTHSpatialStore *spatial_b = hth_spatial_store_create();
    HTHDynamicBodyStore *bodies_a = hth_dynamic_body_store_create();
    HTHDynamicBodyStore *bodies_b = hth_dynamic_body_store_create();
    HTHDynamicBody value_a = body(1.0F, 1.0F, 1.0F,
                                  1.0F, 0.0F, 0.0F);
    HTHDynamicBody value_b = body(2.0F, 2.0F, 2.0F,
                                  2.0F, 0.0F, 0.0F);
    HTHDynamicBody output;
    HTHEntityHandle entity_a;
    HTHEntityHandle entity_b;
    HTHEntityHandle extra;
    size_t index;

    CHECK(entities_a != NULL && entities_b != NULL && spatial_a != NULL &&
          spatial_b != NULL && bodies_a != NULL && bodies_b != NULL);
    CHECK(hth_entity_registry_create_entity(entities_a, &entity_a));
    CHECK(hth_entity_registry_create_entity(entities_b, &entity_b));
    CHECK(hth_entity_handle_equal(entity_a, entity_b));
    CHECK(attach_spatial(spatial_a, entities_a, entity_a,
                         transform(1.0F, 0.0F, 0.0F, 0.0F)));
    CHECK(attach_spatial(spatial_b, entities_b, entity_b,
                         transform(2.0F, 0.0F, 0.0F, 0.0F)));
    CHECK(hth_dynamic_body_attach(
        bodies_a, entities_a, spatial_a, entity_a, &value_a));
    CHECK(hth_dynamic_body_attach(
        bodies_b, entities_b, spatial_b, entity_b, &value_b));
    CHECK(hth_dynamic_body_get(bodies_a, entities_a, entity_a, &output));
    CHECK(body_equal(output, value_a));
    CHECK(hth_dynamic_body_get(bodies_b, entities_b, entity_b, &output));
    CHECK(body_equal(output, value_b));

    for (index = 1U; index < 100U; ++index) {
        CHECK(hth_entity_registry_create_entity(entities_a, &extra));
        CHECK(attach_spatial(spatial_a, entities_a, extra,
                             transform((float)index, 0.0F, 0.0F, 0.0F)));
        CHECK(hth_dynamic_body_attach(
            bodies_a, entities_a, spatial_a, extra, &value_a));
    }
    hth_dynamic_body_store_destroy(bodies_a);
    CHECK(hth_entity_registry_live_count(entities_a) == 100U);
    CHECK(hth_spatial_store_has(spatial_a, entities_a, entity_a));
    hth_dynamic_body_store_destroy(bodies_b);
    CHECK(hth_entity_registry_is_alive(entities_b, entity_b));
    hth_spatial_store_destroy(spatial_a);
    hth_spatial_store_destroy(spatial_b);
    hth_entity_registry_destroy(entities_a);
    hth_entity_registry_destroy(entities_b);
    return true;
}

int main(void)
{
    if (!test_lifecycle_attach_and_invalid_arguments() ||
        !test_non_finite_and_non_positive_validation() ||
        !test_velocity_remove_and_spatial_independence() ||
        !test_dead_stale_and_reused_entity_isolation() ||
        !test_growth_sparse_iteration_and_spatial_independence() ||
        !test_multiple_pairs_and_destroy_with_present_bodies()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
