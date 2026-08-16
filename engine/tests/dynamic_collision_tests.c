#include "dynamic_collision.h"

#include "world.h"

#include <math.h>
#include <stdbool.h>
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

typedef struct {
    HTHEntityRegistry *entities;
    HTHSpatialStore *spatial;
    HTHDynamicBodyStore *bodies;
    HTHEntityHandle entity;
} Fixture;

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

static bool close_float(float left, float right)
{
    return fabsf(left - right) <= 1.0e-5F;
}

static bool close_vector(HTHVec3 left, HTHVec3 right)
{
    return close_float(left.x, right.x) && close_float(left.y, right.y) &&
           close_float(left.z, right.z);
}

static bool result_is_zero(HTHDynamicCollisionResult result)
{
    return !result.moved && !result.collided && !result.start_solid;
}

static HTHCollisionWorld distant_world(void)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] = (HTHAABB){{100.0F, -10.0F, -10.0F},
                                   {101.0F, 10.0F, 10.0F}};
    world.obstacle_count = 1U;
    return world;
}

static HTHCollisionWorld one_obstacle(HTHAABB obstacle)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] = obstacle;
    world.obstacle_count = 1U;
    return world;
}

static bool fixture_init(Fixture *fixture, HTHSpatialTransform spatial_value,
                         HTHDynamicBody body_value)
{
    fixture->entities = hth_entity_registry_create();
    fixture->spatial = hth_spatial_store_create();
    fixture->bodies = hth_dynamic_body_store_create();
    if (fixture->entities == NULL || fixture->spatial == NULL ||
        fixture->bodies == NULL ||
        !hth_entity_registry_create_entity(
            fixture->entities, &fixture->entity) ||
        !hth_spatial_store_attach(
            fixture->spatial, fixture->entities, fixture->entity,
            &spatial_value) ||
        !hth_dynamic_body_attach(
            fixture->bodies, fixture->entities, fixture->spatial,
            fixture->entity, &body_value)) {
        return false;
    }
    return true;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_spatial_store_destroy(fixture->spatial);
    hth_entity_registry_destroy(fixture->entities);
}

static bool fixture_state(Fixture *fixture, HTHSpatialTransform *spatial_value,
                          HTHDynamicBody *body_value)
{
    return hth_spatial_store_get(
               fixture->spatial, fixture->entities, fixture->entity,
               spatial_value) &&
           hth_dynamic_body_get(
               fixture->bodies, fixture->entities, fixture->entity,
               body_value);
}

static bool test_free_motion_zero_velocity_and_zero_dt(void)
{
    Fixture fixture;
    HTHCollisionWorld world = distant_world();
    HTHDynamicCollisionResult result = {true, true, true};
    HTHSpatialTransform spatial_value;
    HTHDynamicBody body_value;

    CHECK(fixture_init(&fixture, transform(0.0F, 1.0F, 0.0F, 0.25F),
                       body(0.5F, 0.5F, 0.5F, 1.0F, 0.0F, 0.0F)));
    CHECK(hth_dynamic_collision_move(
        fixture.bodies, fixture.entities, fixture.spatial, &world,
        fixture.entity, 1.0F, &result));
    CHECK(result.moved && !result.collided && !result.start_solid);
    CHECK(fixture_state(&fixture, &spatial_value, &body_value));
    CHECK(close_vector(spatial_value.position,
                       (HTHVec3){1.0F, 1.0F, 0.0F}));
    CHECK(close_float(spatial_value.yaw, 0.25F));
    CHECK(close_vector(body_value.velocity,
                       (HTHVec3){1.0F, 0.0F, 0.0F}));

    CHECK(hth_dynamic_body_set_velocity(
        fixture.bodies, fixture.entities, fixture.entity,
        (HTHVec3){0.0F, 0.0F, 0.0F}));
    CHECK(hth_dynamic_collision_move(
        fixture.bodies, fixture.entities, fixture.spatial, &world,
        fixture.entity, 2.0F, &result));
    CHECK(result_is_zero(result));
    CHECK(fixture_state(&fixture, &spatial_value, &body_value));
    CHECK(close_vector(spatial_value.position,
                       (HTHVec3){1.0F, 1.0F, 0.0F}));

    CHECK(hth_dynamic_body_set_velocity(
        fixture.bodies, fixture.entities, fixture.entity,
        (HTHVec3){5.0F, 6.0F, 7.0F}));
    CHECK(hth_dynamic_collision_move(
        fixture.bodies, fixture.entities, fixture.spatial, &world,
        fixture.entity, 0.0F, &result));
    CHECK(result_is_zero(result));
    CHECK(fixture_state(&fixture, &spatial_value, &body_value));
    CHECK(close_vector(spatial_value.position,
                       (HTHVec3){1.0F, 1.0F, 0.0F}));
    CHECK(close_vector(body_value.velocity,
                       (HTHVec3){5.0F, 6.0F, 7.0F}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_invalid_calls_are_transactional(void)
{
    Fixture fixture;
    HTHCollisionWorld world = distant_world();
    HTHCollisionWorld invalid_world = {0};
    HTHDynamicCollisionResult result = {true, true, true};
    HTHSpatialTransform before_spatial;
    HTHSpatialTransform after_spatial;
    HTHDynamicBody before_body;
    HTHDynamicBody after_body;
    float invalid_dt[] = {-1.0F, NAN, INFINITY, -INFINITY};
    size_t index;

    CHECK(fixture_init(&fixture, transform(2.0F, 3.0F, 4.0F, 1.0F),
                       body(0.5F, 0.5F, 0.5F, 1.0F, 2.0F, 3.0F)));
    CHECK(fixture_state(&fixture, &before_spatial, &before_body));
    for (index = 0U; index < sizeof(invalid_dt) /
                               sizeof(invalid_dt[0]); ++index) {
        CHECK(!hth_dynamic_collision_move(
            fixture.bodies, fixture.entities, fixture.spatial, &world,
            fixture.entity, invalid_dt[index], &result));
        CHECK(result_is_zero(result));
    }
    CHECK(!hth_dynamic_collision_move(
        fixture.bodies, fixture.entities, fixture.spatial, &invalid_world,
        fixture.entity, 1.0F, &result));
    CHECK(result_is_zero(result));
    CHECK(!hth_dynamic_collision_move(
        NULL, fixture.entities, fixture.spatial, &world,
        fixture.entity, 1.0F, &result));
    CHECK(!hth_dynamic_collision_move(
        fixture.bodies, fixture.entities, fixture.spatial, &world,
        fixture.entity, 1.0F, NULL));
    CHECK(fixture_state(&fixture, &after_spatial, &after_body));
    CHECK(close_vector(after_spatial.position, before_spatial.position));
    CHECK(close_float(after_spatial.yaw, before_spatial.yaw));
    CHECK(close_vector(after_body.half_extents, before_body.half_extents));
    CHECK(close_vector(after_body.velocity, before_body.velocity));
    fixture_destroy(&fixture);
    return true;
}

static bool test_floor_wall_and_ceiling_collision(void)
{
    Fixture floor_fixture;
    Fixture wall_fixture;
    Fixture ceiling_fixture;
    HTHCollisionWorld floor_world = one_obstacle(
        (HTHAABB){{-10.0F, -1.0F, -10.0F},
                  {10.0F, 0.0F, 10.0F}});
    HTHCollisionWorld wall_world = one_obstacle(
        (HTHAABB){{2.0F, -10.0F, -10.0F},
                  {2.2F, 10.0F, 10.0F}});
    HTHCollisionWorld ceiling_world = one_obstacle(
        (HTHAABB){{-10.0F, 2.0F, -10.0F},
                  {10.0F, 3.0F, 10.0F}});
    HTHDynamicCollisionResult result;
    HTHSpatialTransform spatial_value;
    HTHDynamicBody body_value;

    CHECK(fixture_init(&floor_fixture,
                       transform(0.0F, 2.0F, 0.0F, 0.0F),
                       body(0.5F, 0.5F, 0.5F, 0.0F, -3.0F, 0.0F)));
    CHECK(hth_dynamic_collision_move(
        floor_fixture.bodies, floor_fixture.entities, floor_fixture.spatial,
        &floor_world, floor_fixture.entity, 1.0F, &result));
    CHECK(result.moved && result.collided && !result.start_solid);
    CHECK(fixture_state(&floor_fixture, &spatial_value, &body_value));
    CHECK(close_vector(spatial_value.position,
                       (HTHVec3){0.0F, 0.5F, 0.0F}));
    CHECK(close_vector(body_value.velocity,
                       (HTHVec3){0.0F, 0.0F, 0.0F}));

    CHECK(fixture_init(&wall_fixture,
                       transform(0.0F, 0.0F, 0.0F, 0.0F),
                       body(0.5F, 0.5F, 0.5F, 4.0F, 0.0F, 0.0F)));
    CHECK(hth_dynamic_collision_move(
        wall_fixture.bodies, wall_fixture.entities, wall_fixture.spatial,
        &wall_world, wall_fixture.entity, 1.0F, &result));
    CHECK(result.moved && result.collided && !result.start_solid);
    CHECK(fixture_state(&wall_fixture, &spatial_value, &body_value));
    CHECK(close_vector(spatial_value.position,
                       (HTHVec3){1.5F, 0.0F, 0.0F}));
    CHECK(close_vector(body_value.velocity,
                       (HTHVec3){0.0F, 0.0F, 0.0F}));

    CHECK(fixture_init(&ceiling_fixture,
                       transform(0.0F, 0.5F, 0.0F, 0.0F),
                       body(0.5F, 0.5F, 0.5F, 0.0F, 3.0F, 0.0F)));
    CHECK(hth_dynamic_collision_move(
        ceiling_fixture.bodies, ceiling_fixture.entities,
        ceiling_fixture.spatial, &ceiling_world, ceiling_fixture.entity,
        1.0F, &result));
    CHECK(result.moved && result.collided && !result.start_solid);
    CHECK(fixture_state(&ceiling_fixture, &spatial_value, &body_value));
    CHECK(close_vector(spatial_value.position,
                       (HTHVec3){0.0F, 1.5F, 0.0F}));
    CHECK(close_vector(body_value.velocity,
                       (HTHVec3){0.0F, 0.0F, 0.0F}));
    fixture_destroy(&floor_fixture);
    fixture_destroy(&wall_fixture);
    fixture_destroy(&ceiling_fixture);
    return true;
}

static bool test_wall_slide_corner_and_tunneling(void)
{
    Fixture slide_fixture;
    Fixture corner_fixture;
    Fixture fast_fixture;
    HTHCollisionWorld wall_world = one_obstacle(
        (HTHAABB){{2.0F, -10.0F, -10.0F},
                  {2.2F, 10.0F, 10.0F}});
    HTHCollisionWorld corner_world = {0};
    HTHCollisionWorld thin_world = one_obstacle(
        (HTHAABB){{10.0F, -10.0F, -10.0F},
                  {10.1F, 10.0F, 10.0F}});
    HTHDynamicCollisionResult result;
    HTHSpatialTransform spatial_value;
    HTHDynamicBody body_value;

    corner_world.obstacles[0] =
        (HTHAABB){{2.0F, -10.0F, -10.0F}, {2.2F, 10.0F, 10.0F}};
    corner_world.obstacles[1] =
        (HTHAABB){{-10.0F, -10.0F, 2.0F}, {10.0F, 10.0F, 2.2F}};
    corner_world.obstacle_count = 2U;

    CHECK(fixture_init(&slide_fixture,
                       transform(0.0F, 0.0F, 0.0F, 0.0F),
                       body(0.5F, 0.5F, 0.5F, 4.0F, 0.0F, 2.0F)));
    CHECK(hth_dynamic_collision_move(
        slide_fixture.bodies, slide_fixture.entities, slide_fixture.spatial,
        &wall_world, slide_fixture.entity, 1.0F, &result));
    CHECK(result.moved && result.collided && !result.start_solid);
    CHECK(fixture_state(&slide_fixture, &spatial_value, &body_value));
    CHECK(close_vector(spatial_value.position,
                       (HTHVec3){1.5F, 0.0F, 2.0F}));
    CHECK(close_vector(body_value.velocity,
                       (HTHVec3){0.0F, 0.0F, 2.0F}));

    CHECK(fixture_init(&corner_fixture,
                       transform(0.0F, 0.0F, 0.0F, 0.0F),
                       body(0.5F, 0.5F, 0.5F, 4.0F, 0.0F, 4.0F)));
    CHECK(hth_dynamic_collision_move(
        corner_fixture.bodies, corner_fixture.entities,
        corner_fixture.spatial, &corner_world, corner_fixture.entity,
        1.0F, &result));
    CHECK(result.moved && result.collided && !result.start_solid);
    CHECK(fixture_state(&corner_fixture, &spatial_value, &body_value));
    CHECK(close_vector(spatial_value.position,
                       (HTHVec3){1.5F, 0.0F, 1.5F}));
    CHECK(close_vector(body_value.velocity,
                       (HTHVec3){0.0F, 0.0F, 0.0F}));
    CHECK(isfinite(spatial_value.position.x) &&
          isfinite(spatial_value.position.y) &&
          isfinite(spatial_value.position.z));

    CHECK(fixture_init(&fast_fixture,
                       transform(0.0F, 0.0F, 0.0F, 0.0F),
                       body(0.5F, 0.5F, 0.5F, 1000.0F, 0.0F, 0.0F)));
    CHECK(hth_dynamic_collision_move(
        fast_fixture.bodies, fast_fixture.entities, fast_fixture.spatial,
        &thin_world, fast_fixture.entity, 1.0F, &result));
    CHECK(result.moved && result.collided && !result.start_solid);
    CHECK(fixture_state(&fast_fixture, &spatial_value, &body_value));
    CHECK(close_float(spatial_value.position.x, 9.5F));
    CHECK(spatial_value.position.x + body_value.half_extents.x <= 10.0F);
    CHECK(close_vector(body_value.velocity,
                       (HTHVec3){0.0F, 0.0F, 0.0F}));
    fixture_destroy(&slide_fixture);
    fixture_destroy(&corner_fixture);
    fixture_destroy(&fast_fixture);
    return true;
}

static bool test_start_solid_no_depenetration(void)
{
    Fixture fixture;
    HTHCollisionWorld world = one_obstacle(
        (HTHAABB){{-1.0F, -1.0F, -1.0F}, {1.0F, 1.0F, 1.0F}});
    HTHDynamicCollisionResult result;
    HTHSpatialTransform spatial_value;
    HTHDynamicBody body_value;

    CHECK(fixture_init(&fixture, transform(0.0F, 0.0F, 0.0F, 0.75F),
                       body(0.5F, 0.5F, 0.5F, 3.0F, 4.0F, 5.0F)));
    CHECK(hth_dynamic_collision_move(
        fixture.bodies, fixture.entities, fixture.spatial, &world,
        fixture.entity, 1.0F, &result));
    CHECK(!result.moved && result.collided && result.start_solid);
    CHECK(fixture_state(&fixture, &spatial_value, &body_value));
    CHECK(close_vector(spatial_value.position,
                       (HTHVec3){0.0F, 0.0F, 0.0F}));
    CHECK(close_float(spatial_value.yaw, 0.75F));
    CHECK(close_vector(body_value.velocity,
                       (HTHVec3){3.0F, 4.0F, 5.0F}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_missing_spatial_missing_body_and_stale_entity(void)
{
    Fixture fixture;
    HTHCollisionWorld world = distant_world();
    HTHDynamicCollisionResult result = {true, true, true};
    HTHDynamicBody before_body;
    HTHDynamicBody after_body;
    HTHSpatialTransform replacement_transform =
        transform(20.0F, 0.0F, 0.0F, 0.0F);
    HTHDynamicBody replacement_body =
        body(0.5F, 0.5F, 0.5F, 2.0F, 0.0F, 0.0F);
    HTHEntityHandle stale;
    HTHEntityHandle replacement;

    CHECK(fixture_init(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       body(0.5F, 0.5F, 0.5F, 1.0F, 0.0F, 0.0F)));
    CHECK(hth_dynamic_body_get(
        fixture.bodies, fixture.entities, fixture.entity, &before_body));
    CHECK(hth_spatial_store_remove(
        fixture.spatial, fixture.entities, fixture.entity));
    CHECK(hth_dynamic_body_has(
        fixture.bodies, fixture.entities, fixture.entity));
    CHECK(!hth_dynamic_collision_move(
        fixture.bodies, fixture.entities, fixture.spatial, &world,
        fixture.entity, 1.0F, &result));
    CHECK(result_is_zero(result));
    CHECK(hth_dynamic_body_get(
        fixture.bodies, fixture.entities, fixture.entity, &after_body));
    CHECK(close_vector(after_body.velocity, before_body.velocity));

    CHECK(hth_spatial_store_attach(
        fixture.spatial, fixture.entities, fixture.entity,
        &replacement_transform));
    CHECK(hth_dynamic_collision_move(
        fixture.bodies, fixture.entities, fixture.spatial, &world,
        fixture.entity, 1.0F, &result));
    CHECK(result.moved && !result.collided && !result.start_solid);

    stale = fixture.entity;
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));
    CHECK(hth_entity_registry_create_entity(
        fixture.entities, &replacement));
    CHECK(replacement.index == stale.index &&
          replacement.generation != stale.generation);
    CHECK(hth_spatial_store_attach(
        fixture.spatial, fixture.entities, replacement,
        &replacement_transform));
    CHECK(hth_dynamic_body_attach(
        fixture.bodies, fixture.entities, fixture.spatial, replacement,
        &replacement_body));
    CHECK(!hth_dynamic_collision_move(
        fixture.bodies, fixture.entities, fixture.spatial, &world,
        stale, 1.0F, &result));
    CHECK(result_is_zero(result));
    CHECK(hth_dynamic_body_get(
        fixture.bodies, fixture.entities, replacement, &after_body));
    CHECK(close_vector(after_body.velocity, replacement_body.velocity));

    CHECK(hth_dynamic_body_remove(
        fixture.bodies, fixture.entities, replacement));
    CHECK(!hth_dynamic_collision_move(
        fixture.bodies, fixture.entities, fixture.spatial, &world,
        replacement, 1.0F, &result));
    CHECK(result_is_zero(result));
    fixture_destroy(&fixture);
    return true;
}

static bool test_spatial_authority_reattach_and_yaw_ignored(void)
{
    Fixture fixture;
    HTHCollisionWorld world = distant_world();
    HTHDynamicCollisionResult first_result;
    HTHDynamicCollisionResult second_result;
    HTHSpatialTransform spatial_value;
    HTHDynamicBody body_value;
    HTHSpatialTransform relocated = transform(5.0F, 1.0F, 0.0F, 100.0F);
    HTHVec3 velocity = {1.0F, 0.0F, 0.0F};

    CHECK(fixture_init(&fixture, transform(0.0F, 1.0F, 0.0F, -7.0F),
                       body(0.5F, 0.5F, 0.5F, 1.0F, 0.0F, 0.0F)));
    CHECK(hth_spatial_store_set(
        fixture.spatial, fixture.entities, fixture.entity, &relocated));
    CHECK(hth_dynamic_collision_move(
        fixture.bodies, fixture.entities, fixture.spatial, &world,
        fixture.entity, 1.0F, &first_result));
    CHECK(fixture_state(&fixture, &spatial_value, &body_value));
    CHECK(close_vector(spatial_value.position,
                       (HTHVec3){6.0F, 1.0F, 0.0F}));
    CHECK(close_float(spatial_value.yaw, 100.0F));

    CHECK(hth_spatial_store_remove(
        fixture.spatial, fixture.entities, fixture.entity));
    relocated = transform(10.0F, 1.0F, 0.0F, -100.0F);
    CHECK(hth_spatial_store_attach(
        fixture.spatial, fixture.entities, fixture.entity, &relocated));
    CHECK(hth_dynamic_body_get(
        fixture.bodies, fixture.entities, fixture.entity, &body_value));
    CHECK(close_vector(body_value.velocity, velocity));
    CHECK(hth_dynamic_collision_move(
        fixture.bodies, fixture.entities, fixture.spatial, &world,
        fixture.entity, 1.0F, &second_result));
    CHECK(first_result.moved == second_result.moved &&
          first_result.collided == second_result.collided &&
          first_result.start_solid == second_result.start_solid);
    CHECK(fixture_state(&fixture, &spatial_value, &body_value));
    CHECK(close_vector(spatial_value.position,
                       (HTHVec3){11.0F, 1.0F, 0.0F}));
    CHECK(close_float(spatial_value.yaw, -100.0F));
    CHECK(close_vector(body_value.half_extents,
                       (HTHVec3){0.5F, 0.5F, 0.5F}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_yaw_does_not_rotate_collision_shape(void)
{
    Fixture zero_yaw;
    Fixture large_yaw;
    HTHCollisionWorld world = one_obstacle(
        (HTHAABB){{2.0F, -10.0F, -10.0F},
                  {2.2F, 10.0F, 10.0F}});
    HTHDynamicCollisionResult zero_result;
    HTHDynamicCollisionResult large_result;
    HTHSpatialTransform zero_spatial;
    HTHSpatialTransform large_spatial;
    HTHDynamicBody zero_body_value;
    HTHDynamicBody large_body_value;

    CHECK(fixture_init(&zero_yaw, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       body(0.5F, 0.5F, 0.5F, 4.0F, 0.0F, 1.0F)));
    CHECK(fixture_init(&large_yaw,
                       transform(0.0F, 0.0F, 0.0F, 123.0F),
                       body(0.5F, 0.5F, 0.5F, 4.0F, 0.0F, 1.0F)));
    CHECK(hth_dynamic_collision_move(
        zero_yaw.bodies, zero_yaw.entities, zero_yaw.spatial, &world,
        zero_yaw.entity, 1.0F, &zero_result));
    CHECK(hth_dynamic_collision_move(
        large_yaw.bodies, large_yaw.entities, large_yaw.spatial, &world,
        large_yaw.entity, 1.0F, &large_result));
    CHECK(zero_result.moved == large_result.moved &&
          zero_result.collided == large_result.collided &&
          zero_result.start_solid == large_result.start_solid);
    CHECK(fixture_state(&zero_yaw, &zero_spatial, &zero_body_value));
    CHECK(fixture_state(&large_yaw, &large_spatial, &large_body_value));
    CHECK(close_vector(zero_spatial.position, large_spatial.position));
    CHECK(close_float(zero_spatial.yaw, 0.0F));
    CHECK(close_float(large_spatial.yaw, 123.0F));
    CHECK(close_vector(zero_body_value.velocity,
                       large_body_value.velocity));
    fixture_destroy(&zero_yaw);
    fixture_destroy(&large_yaw);
    return true;
}

static bool test_no_gravity_no_step_and_render_wedge_pass_through(void)
{
    Fixture stationary_fixture;
    Fixture step_fixture;
    Fixture wedge_fixture;
    HTHCollisionWorld floor_world = one_obstacle(
        (HTHAABB){{-10.0F, -1.0F, -10.0F},
                  {10.0F, 0.0F, 10.0F}});
    HTHCollisionWorld step_world = one_obstacle(
        (HTHAABB){{2.0F, -1.0F, -1.0F}, {3.0F, 0.6F, 1.0F}});
    HTHCollisionWorld wedge_collision;
    HTHDynamicCollisionResult result;
    HTHSpatialTransform spatial_value;
    HTHDynamicBody body_value;
    HTHWorld source;

    CHECK(fixture_init(&stationary_fixture,
                       transform(0.0F, 2.0F, 0.0F, 0.0F),
                       body(0.5F, 0.5F, 0.5F, 0.0F, 0.0F, 0.0F)));
    CHECK(hth_dynamic_collision_move(
        stationary_fixture.bodies, stationary_fixture.entities,
        stationary_fixture.spatial, &floor_world,
        stationary_fixture.entity, 1.0F, &result));
    CHECK(result_is_zero(result));
    CHECK(fixture_state(
        &stationary_fixture, &spatial_value, &body_value));
    CHECK(close_vector(spatial_value.position,
                       (HTHVec3){0.0F, 2.0F, 0.0F}));

    CHECK(fixture_init(&step_fixture,
                       transform(0.0F, 0.5F, 0.0F, 0.0F),
                       body(0.5F, 0.5F, 0.5F, 4.0F, 0.0F, 0.0F)));
    CHECK(hth_dynamic_collision_move(
        step_fixture.bodies, step_fixture.entities, step_fixture.spatial,
        &step_world, step_fixture.entity, 1.0F, &result));
    CHECK(result.moved && result.collided);
    CHECK(fixture_state(&step_fixture, &spatial_value, &body_value));
    CHECK(close_vector(spatial_value.position,
                       (HTHVec3){1.5F, 0.5F, 0.0F}));
    CHECK(close_float(body_value.velocity.x, 0.0F));

    CHECK(hth_world_init(&source));
    CHECK(hth_world_add_static_object(
        &source, (HTHAABB){{2.0F, -1.0F, -1.0F}, {3.0F, 1.0F, 1.0F}},
        HTH_WORLD_COLLISION_NONE, HTH_WORLD_RENDER_WEDGE,
        HTH_WORLD_OBJECT_VISIBLE, HTH_WORLD_VISUAL_BOX));
    CHECK(hth_world_add_static_object(
        &source,
        (HTHAABB){{100.0F, -10.0F, -10.0F}, {101.0F, 10.0F, 10.0F}},
        HTH_WORLD_COLLISION_AABB, HTH_WORLD_RENDER_NONE,
        HTH_WORLD_OBJECT_COLLIDABLE, HTH_WORLD_VISUAL_NONE));
    CHECK(hth_world_finalize(&source));
    CHECK(hth_collision_world_build_from_world(&wedge_collision, &source));
    CHECK(wedge_collision.obstacle_count == 1U);
    CHECK(fixture_init(&wedge_fixture,
                       transform(0.0F, 0.0F, 0.0F, 0.0F),
                       body(0.5F, 0.5F, 0.5F, 5.0F, 0.0F, 0.0F)));
    CHECK(hth_dynamic_collision_move(
        wedge_fixture.bodies, wedge_fixture.entities, wedge_fixture.spatial,
        &wedge_collision, wedge_fixture.entity, 1.0F, &result));
    CHECK(result.moved && !result.collided);
    CHECK(fixture_state(&wedge_fixture, &spatial_value, &body_value));
    CHECK(close_float(spatial_value.position.x, 5.0F));
    fixture_destroy(&stationary_fixture);
    fixture_destroy(&step_fixture);
    fixture_destroy(&wedge_fixture);
    hth_world_shutdown(&source);
    return true;
}

int main(void)
{
    if (!test_free_motion_zero_velocity_and_zero_dt() ||
        !test_invalid_calls_are_transactional() ||
        !test_floor_wall_and_ceiling_collision() ||
        !test_wall_slide_corner_and_tunneling() ||
        !test_start_solid_no_depenetration() ||
        !test_missing_spatial_missing_body_and_stale_entity() ||
        !test_spatial_authority_reattach_and_yaw_ignored() ||
        !test_yaw_does_not_rotate_collision_shape() ||
        !test_no_gravity_no_step_and_render_wedge_pass_through()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
