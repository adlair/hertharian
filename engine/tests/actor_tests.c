#include "actor.h"

#include "collision_world.h"
#include "dynamic_body.h"
#include "dynamic_collision.h"
#include "spatial.h"

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

static bool transform_equal(HTHSpatialTransform left,
                            HTHSpatialTransform right)
{
    return vector_equal(left.position, right.position) &&
           left.yaw == right.yaw;
}

static bool body_equal(HTHDynamicBody left, HTHDynamicBody right)
{
    return vector_equal(left.half_extents, right.half_extents) &&
           vector_equal(left.velocity, right.velocity);
}

static bool test_store_lifecycle_attach_presence_and_remove(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHActorStore *actors = hth_actor_store_create();
    HTHEntityHandle entity;
    HTHEntityHandle dead;
    HTHEntityHandle invalid = hth_entity_handle_invalid();

    CHECK(entities != NULL && actors != NULL);
    CHECK(hth_entity_registry_create_entity(entities, &entity));
    CHECK(hth_entity_registry_create_entity(entities, &dead));
    CHECK(hth_entity_registry_destroy_entity(entities, dead));

    CHECK(!hth_actor_store_attach(NULL, entities, entity));
    CHECK(!hth_actor_store_attach(actors, NULL, entity));
    CHECK(!hth_actor_store_attach(actors, entities, invalid));
    CHECK(!hth_actor_store_attach(actors, entities, dead));
    CHECK(!hth_actor_store_has(NULL, entities, entity));
    CHECK(!hth_actor_store_has(actors, NULL, entity));
    CHECK(!hth_actor_store_has(actors, entities, invalid));
    CHECK(!hth_actor_store_remove(NULL, entities, entity));
    CHECK(!hth_actor_store_remove(actors, NULL, entity));

    /* Actor requires only a live Entity: no Spatial or Body Store exists. */
    CHECK(hth_actor_store_attach(actors, entities, entity));
    CHECK(hth_actor_store_has(actors, entities, entity));
    CHECK(!hth_actor_store_attach(actors, entities, entity));
    CHECK(hth_actor_store_has(actors, entities, entity));
    CHECK(hth_actor_store_remove(actors, entities, entity));
    CHECK(!hth_actor_store_has(actors, entities, entity));
    CHECK(hth_entity_registry_is_alive(entities, entity));
    CHECK(!hth_actor_store_remove(actors, entities, entity));

    hth_actor_store_destroy(actors);
    hth_actor_store_destroy(NULL);
    CHECK(hth_entity_registry_is_alive(entities, entity));
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_store_removal_independence_and_composition_order(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *spatial = hth_spatial_store_create();
    HTHDynamicBodyStore *bodies = hth_dynamic_body_store_create();
    HTHActorStore *actors = hth_actor_store_create();
    HTHSpatialTransform first_transform =
        transform(1.0F, 2.0F, 3.0F, 0.75F);
    HTHDynamicBody first_body =
        body(0.5F, 1.0F, 0.75F, 2.0F, 3.0F, 4.0F);
    HTHSpatialTransform output_transform;
    HTHDynamicBody output_body;
    HTHEntityHandle actor_first;
    HTHEntityHandle actor_last;
    HTHEntityHandle without_body;

    CHECK(entities != NULL && spatial != NULL && bodies != NULL &&
          actors != NULL);
    CHECK(hth_entity_registry_create_entity(entities, &actor_first));
    CHECK(hth_actor_store_attach(actors, entities, actor_first));
    CHECK(hth_spatial_store_attach(
        spatial, entities, actor_first, &first_transform));
    CHECK(hth_dynamic_body_attach(
        bodies, entities, spatial, actor_first, &first_body));
    CHECK(hth_actor_store_has(actors, entities, actor_first));
    CHECK(hth_spatial_store_has(spatial, entities, actor_first));
    CHECK(hth_dynamic_body_has(bodies, entities, actor_first));

    CHECK(hth_actor_store_remove(actors, entities, actor_first));
    CHECK(hth_entity_registry_is_alive(entities, actor_first));
    CHECK(hth_spatial_store_get(
        spatial, entities, actor_first, &output_transform));
    CHECK(transform_equal(output_transform, first_transform));
    CHECK(hth_dynamic_body_get(
        bodies, entities, actor_first, &output_body));
    CHECK(body_equal(output_body, first_body));

    CHECK(hth_actor_store_attach(actors, entities, actor_first));
    CHECK(hth_spatial_store_remove(spatial, entities, actor_first));
    CHECK(hth_actor_store_has(actors, entities, actor_first));
    CHECK(hth_dynamic_body_has(bodies, entities, actor_first));
    CHECK(hth_spatial_store_attach(
        spatial, entities, actor_first, &first_transform));
    CHECK(hth_actor_store_has(actors, entities, actor_first));
    CHECK(hth_dynamic_body_remove(bodies, entities, actor_first));
    CHECK(hth_actor_store_has(actors, entities, actor_first));
    CHECK(hth_spatial_store_has(spatial, entities, actor_first));

    CHECK(hth_entity_registry_create_entity(entities, &actor_last));
    CHECK(hth_spatial_store_attach(
        spatial, entities, actor_last, &first_transform));
    CHECK(hth_dynamic_body_attach(
        bodies, entities, spatial, actor_last, &first_body));
    CHECK(hth_actor_store_attach(actors, entities, actor_last));
    CHECK(hth_actor_store_has(actors, entities, actor_last));
    CHECK(hth_spatial_store_has(spatial, entities, actor_last));
    CHECK(hth_dynamic_body_has(bodies, entities, actor_last));

    CHECK(hth_entity_registry_create_entity(entities, &without_body));
    CHECK(hth_spatial_store_attach(
        spatial, entities, without_body, &first_transform));
    CHECK(!hth_dynamic_body_has(bodies, entities, without_body));
    CHECK(hth_actor_store_attach(actors, entities, without_body));
    CHECK(hth_actor_store_has(actors, entities, without_body));
    CHECK(!hth_dynamic_body_has(bodies, entities, without_body));

    hth_actor_store_destroy(actors);
    hth_dynamic_body_store_destroy(bodies);
    hth_spatial_store_destroy(spatial);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_entity_death_reuse_and_stale_remove(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHActorStore *actors = hth_actor_store_create();
    HTHEntityHandle stale;
    HTHEntityHandle current;

    CHECK(entities != NULL && actors != NULL);
    CHECK(hth_entity_registry_create_entity(entities, &stale));
    CHECK(hth_actor_store_attach(actors, entities, stale));
    CHECK(hth_entity_registry_destroy_entity(entities, stale));
    CHECK(!hth_actor_store_has(actors, entities, stale));
    CHECK(!hth_actor_store_remove(actors, entities, stale));
    CHECK(!hth_actor_store_attach(actors, entities, stale));

    CHECK(hth_entity_registry_create_entity(entities, &current));
    CHECK(current.index == stale.index);
    CHECK(current.generation == stale.generation + 1U);
    CHECK(!hth_actor_store_has(actors, entities, current));
    CHECK(!hth_actor_store_remove(actors, entities, current));
    CHECK(hth_actor_store_attach(actors, entities, current));
    CHECK(hth_actor_store_has(actors, entities, current));
    CHECK(!hth_actor_store_remove(actors, entities, stale));
    CHECK(hth_actor_store_has(actors, entities, current));

    hth_actor_store_destroy(actors);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_growth_sparse_iteration_and_reuse(void)
{
    enum { ENTITY_COUNT = 130 };
    static const uint32_t actor_indices[] = {1U, 7U, 65U, 129U};
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHActorStore *actors = hth_actor_store_create();
    HTHEntityHandle handles[ENTITY_COUNT];
    HTHActorIterator iterator;
    HTHEntityHandle iterated = {0U, 1U};
    HTHEntityHandle replacement;
    size_t index;

    CHECK(entities != NULL && actors != NULL);
    hth_actor_iterator_begin(&iterator);
    CHECK(!hth_actor_iterator_next(
        actors, entities, &iterator, &iterated));
    CHECK(hth_entity_handle_equal(iterated, hth_entity_handle_invalid()));
    CHECK(!hth_actor_iterator_next(NULL, entities, &iterator, &iterated));
    CHECK(!hth_actor_iterator_next(actors, NULL, &iterator, &iterated));
    CHECK(!hth_actor_iterator_next(actors, entities, NULL, &iterated));
    CHECK(!hth_actor_iterator_next(actors, entities, &iterator, NULL));
    hth_actor_iterator_begin(NULL);

    for (index = 0U; index < ENTITY_COUNT; ++index) {
        CHECK(hth_entity_registry_create_entity(entities, &handles[index]));
    }
    for (index = 0U; index < sizeof(actor_indices) /
                               sizeof(actor_indices[0]); ++index) {
        CHECK(hth_actor_store_attach(
            actors, entities, handles[actor_indices[index]]));
    }
    for (index = 0U; index < sizeof(actor_indices) /
                               sizeof(actor_indices[0]); ++index) {
        CHECK(hth_actor_store_has(
            actors, entities, handles[actor_indices[index]]));
    }

    hth_actor_iterator_begin(&iterator);
    for (index = 0U; index < sizeof(actor_indices) /
                               sizeof(actor_indices[0]); ++index) {
        CHECK(hth_actor_iterator_next(
            actors, entities, &iterator, &iterated));
        CHECK(hth_entity_handle_equal(
            iterated, handles[actor_indices[index]]));
    }
    CHECK(!hth_actor_iterator_next(
        actors, entities, &iterator, &iterated));
    CHECK(hth_entity_handle_equal(iterated, hth_entity_handle_invalid()));

    CHECK(hth_actor_store_remove(actors, entities, handles[1]));
    CHECK(hth_entity_registry_destroy_entity(entities, handles[65]));
    CHECK(hth_entity_registry_create_entity(entities, &replacement));
    CHECK(replacement.index == 65U);
    CHECK(!hth_actor_store_has(actors, entities, replacement));
    hth_actor_iterator_begin(&iterator);
    CHECK(hth_actor_iterator_next(actors, entities, &iterator, &iterated));
    CHECK(iterated.index == 7U);
    CHECK(hth_actor_iterator_next(actors, entities, &iterator, &iterated));
    CHECK(iterated.index == 129U);
    CHECK(!hth_actor_iterator_next(
        actors, entities, &iterator, &iterated));

    CHECK(hth_actor_store_attach(actors, entities, replacement));
    hth_actor_iterator_begin(&iterator);
    CHECK(hth_actor_iterator_next(actors, entities, &iterator, &iterated));
    CHECK(iterated.index == 7U);
    CHECK(hth_actor_iterator_next(actors, entities, &iterator, &iterated));
    CHECK(hth_entity_handle_equal(iterated, replacement));
    CHECK(hth_actor_iterator_next(actors, entities, &iterator, &iterated));
    CHECK(iterated.index == 129U);
    CHECK(!hth_actor_iterator_next(
        actors, entities, &iterator, &iterated));

    hth_actor_store_destroy(actors);
    CHECK(hth_entity_registry_live_count(entities) == ENTITY_COUNT);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_multiple_store_registry_pairs(void)
{
    HTHEntityRegistry *entities_a = hth_entity_registry_create();
    HTHEntityRegistry *entities_b = hth_entity_registry_create();
    HTHActorStore *actors_a = hth_actor_store_create();
    HTHActorStore *actors_b = hth_actor_store_create();
    HTHEntityHandle entity_a;
    HTHEntityHandle entity_b;

    CHECK(entities_a != NULL && entities_b != NULL &&
          actors_a != NULL && actors_b != NULL);
    CHECK(hth_entity_registry_create_entity(entities_a, &entity_a));
    CHECK(hth_entity_registry_create_entity(entities_b, &entity_b));
    CHECK(hth_entity_handle_equal(entity_a, entity_b));
    CHECK(hth_actor_store_attach(actors_a, entities_a, entity_a));
    CHECK(!hth_actor_store_has(actors_b, entities_b, entity_b));
    CHECK(hth_actor_store_attach(actors_b, entities_b, entity_b));
    CHECK(hth_actor_store_remove(actors_a, entities_a, entity_a));
    CHECK(!hth_actor_store_has(actors_a, entities_a, entity_a));
    CHECK(hth_actor_store_has(actors_b, entities_b, entity_b));

    hth_actor_store_destroy(actors_a);
    hth_actor_store_destroy(actors_b);
    CHECK(hth_entity_registry_is_alive(entities_a, entity_a));
    CHECK(hth_entity_registry_is_alive(entities_b, entity_b));
    hth_entity_registry_destroy(entities_a);
    hth_entity_registry_destroy(entities_b);
    return true;
}

static bool test_actor_removal_does_not_block_dynamic_collision(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *spatial = hth_spatial_store_create();
    HTHDynamicBodyStore *bodies = hth_dynamic_body_store_create();
    HTHActorStore *actors = hth_actor_store_create();
    HTHSpatialTransform initial = transform(0.0F, 0.0F, 0.0F, 1.25F);
    HTHDynamicBody initial_body =
        body(0.5F, 0.5F, 0.5F, 2.0F, 0.0F, 0.0F);
    HTHCollisionWorld collision_world = {0};
    HTHDynamicCollisionResult result;
    HTHSpatialTransform resolved;
    HTHDynamicBody resolved_body;
    HTHEntityHandle entity;

    CHECK(entities != NULL && spatial != NULL && bodies != NULL &&
          actors != NULL);
    collision_world.obstacles[0] =
        (HTHAABB){{100.0F, -10.0F, -10.0F},
                  {101.0F, 10.0F, 10.0F}};
    collision_world.obstacle_count = 1U;
    CHECK(hth_collision_world_is_valid(&collision_world));
    CHECK(hth_entity_registry_create_entity(entities, &entity));
    CHECK(hth_spatial_store_attach(spatial, entities, entity, &initial));
    CHECK(hth_dynamic_body_attach(
        bodies, entities, spatial, entity, &initial_body));
    CHECK(hth_actor_store_attach(actors, entities, entity));
    CHECK(hth_actor_store_remove(actors, entities, entity));
    CHECK(hth_dynamic_collision_move(
        bodies, entities, spatial, &collision_world, entity, 1.0F,
        &result));
    CHECK(result.moved && !result.collided && !result.start_solid);
    CHECK(hth_spatial_store_get(spatial, entities, entity, &resolved));
    CHECK(vector_equal(resolved.position, (HTHVec3){2.0F, 0.0F, 0.0F}));
    CHECK(resolved.yaw == initial.yaw);
    CHECK(hth_dynamic_body_get(bodies, entities, entity, &resolved_body));
    CHECK(body_equal(resolved_body, initial_body));
    CHECK(!hth_actor_store_has(actors, entities, entity));

    hth_actor_store_destroy(actors);
    hth_dynamic_body_store_destroy(bodies);
    hth_spatial_store_destroy(spatial);
    hth_entity_registry_destroy(entities);
    return true;
}

static bool test_composite_entity_death(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHSpatialStore *spatial = hth_spatial_store_create();
    HTHDynamicBodyStore *bodies = hth_dynamic_body_store_create();
    HTHActorStore *actors = hth_actor_store_create();
    HTHSpatialTransform spatial_value =
        transform(4.0F, 5.0F, 6.0F, -0.5F);
    HTHDynamicBody body_value =
        body(1.0F, 2.0F, 3.0F, -1.0F, -2.0F, -3.0F);
    HTHEntityHandle entity;

    CHECK(entities != NULL && spatial != NULL && bodies != NULL &&
          actors != NULL);
    CHECK(hth_entity_registry_create_entity(entities, &entity));
    CHECK(hth_spatial_store_attach(
        spatial, entities, entity, &spatial_value));
    CHECK(hth_dynamic_body_attach(
        bodies, entities, spatial, entity, &body_value));
    CHECK(hth_actor_store_attach(actors, entities, entity));
    CHECK(hth_entity_registry_is_alive(entities, entity));
    CHECK(hth_spatial_store_has(spatial, entities, entity));
    CHECK(hth_dynamic_body_has(bodies, entities, entity));
    CHECK(hth_actor_store_has(actors, entities, entity));

    CHECK(hth_entity_registry_destroy_entity(entities, entity));
    CHECK(!hth_entity_registry_is_alive(entities, entity));
    CHECK(!hth_spatial_store_has(spatial, entities, entity));
    CHECK(!hth_dynamic_body_has(bodies, entities, entity));
    CHECK(!hth_actor_store_has(actors, entities, entity));

    hth_actor_store_destroy(actors);
    hth_dynamic_body_store_destroy(bodies);
    hth_spatial_store_destroy(spatial);
    hth_entity_registry_destroy(entities);
    return true;
}

int main(void)
{
    if (!test_store_lifecycle_attach_presence_and_remove() ||
        !test_store_removal_independence_and_composition_order() ||
        !test_entity_death_reuse_and_stale_remove() ||
        !test_growth_sparse_iteration_and_reuse() ||
        !test_multiple_store_registry_pairs() ||
        !test_actor_removal_does_not_block_dynamic_collision() ||
        !test_composite_entity_death()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
