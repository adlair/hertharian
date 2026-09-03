#include "enemy_chase.h"

#include "enemy_decision.h"
#include "enemy_seek.h"
#include "enemy_target_selection.h"
#include "health.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    HTHActorStore *actors;
    HTHEnemyStore *enemies;
    HTHEnemyTargetStore *targets;
    HTHSpatialStore *spatial;
    HTHDynamicBodyStore *bodies;
    HTHHealthStore *health;
    HTHEntityHandle enemy;
} Fixture;

static HTHSpatialTransform transform(float x, float y, float z, float yaw)
{
    HTHSpatialTransform value = {{x, y, z}, yaw};

    return value;
}

static HTHDynamicBody body(float vx, float vy, float vz)
{
    HTHDynamicBody value = {{0.5F, 0.5F, 0.5F}, {vx, vy, vz}};

    return value;
}

static HTHCollisionWorld distant_world(void)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] = (HTHAABB){{100.0F, -10.0F, -10.0F},
                                   {101.0F, 10.0F, 10.0F}};
    world.obstacle_count = 1U;
    return world;
}

static HTHCollisionWorld wall_world(void)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] = (HTHAABB){{2.0F, -10.0F, -10.0F},
                                   {2.2F, 10.0F, 10.0F}};
    world.obstacle_count = 1U;
    return world;
}

static HTHCollisionWorld start_solid_world(void)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] =
        (HTHAABB){{-1.0F, -1.0F, -1.0F}, {1.0F, 1.0F, 1.0F}};
    world.obstacle_count = 1U;
    return world;
}

static bool fixture_create(Fixture *fixture, HTHSpatialTransform spatial_value,
                           HTHDynamicBody body_value)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->enemies = hth_enemy_store_create();
    fixture->targets = hth_enemy_target_store_create();
    fixture->spatial = hth_spatial_store_create();
    fixture->bodies = hth_dynamic_body_store_create();
    fixture->health = hth_health_store_create();
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->enemies != NULL && fixture->targets != NULL &&
           fixture->spatial != NULL && fixture->bodies != NULL &&
           fixture->health != NULL &&
           hth_entity_registry_create_entity(fixture->entities,
                                             &fixture->enemy) &&
           hth_actor_store_attach(fixture->actors, fixture->entities,
                                  fixture->enemy) &&
           hth_enemy_store_attach(fixture->enemies, fixture->entities,
                                  fixture->actors, fixture->enemy) &&
           hth_spatial_store_attach(fixture->spatial, fixture->entities,
                                    fixture->enemy, &spatial_value) &&
           hth_dynamic_body_attach(fixture->bodies, fixture->entities,
                                   fixture->spatial, fixture->enemy,
                                   &body_value);
}

static void fixture_destroy(Fixture *fixture)
{
    hth_health_store_destroy(fixture->health);
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_spatial_store_destroy(fixture->spatial);
    hth_enemy_target_store_destroy(fixture->targets);
    hth_enemy_store_destroy(fixture->enemies);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static bool create_spatial_entity(Fixture *fixture,
                                  HTHSpatialTransform spatial_value,
                                  HTHEntityHandle *out_entity)
{
    return hth_entity_registry_create_entity(fixture->entities, out_entity) &&
           hth_spatial_store_attach(fixture->spatial, fixture->entities,
                                    *out_entity, &spatial_value);
}

static bool chase(Fixture *fixture, const HTHCollisionWorld *world,
                  HTHVec3 direction, float speed, float dt,
                  HTHDynamicCollisionResult *out_result)
{
    return hth_enemy_chase_apply(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->spatial, fixture->bodies, world, fixture->enemy,
        direction, speed, dt, out_result);
}

static bool close_float(float left, float right)
{
    return fabsf(left - right) <= 0.00001F;
}

static bool close_vector(HTHVec3 left, HTHVec3 right)
{
    return close_float(left.x, right.x) && close_float(left.y, right.y) &&
           close_float(left.z, right.z);
}

static bool result_is_empty(HTHDynamicCollisionResult result)
{
    return !result.moved && !result.collided && !result.start_solid;
}

static bool fixture_state(Fixture *fixture, HTHSpatialTransform *spatial_value,
                          HTHDynamicBody *body_value)
{
    return hth_spatial_store_get(fixture->spatial, fixture->entities,
                                 fixture->enemy, spatial_value) &&
           hth_dynamic_body_get(fixture->bodies, fixture->entities,
                                fixture->enemy, body_value);
}

static bool state_unchanged(Fixture *fixture,
                            HTHSpatialTransform before_spatial,
                            HTHDynamicBody before_body)
{
    HTHSpatialTransform after_spatial;
    HTHDynamicBody after_body;

    return fixture_state(fixture, &after_spatial, &after_body) &&
           memcmp(&before_spatial, &after_spatial,
                  sizeof(before_spatial)) == 0 &&
           memcmp(&before_body, &after_body, sizeof(before_body)) == 0;
}

static bool reset_state(Fixture *fixture, HTHSpatialTransform spatial_value,
                        HTHDynamicBody body_value)
{
    return hth_spatial_store_set(fixture->spatial, fixture->entities,
                                 fixture->enemy, &spatial_value) &&
           hth_dynamic_body_set_velocity(fixture->bodies, fixture->entities,
                                         fixture->enemy,
                                         body_value.velocity);
}

static bool test_argument_failures_are_transactional(void)
{
    Fixture fixture;
    HTHCollisionWorld world = distant_world();
    HTHSpatialTransform before_spatial;
    HTHDynamicBody before_body;
    HTHDynamicCollisionResult result;
    HTHVec3 direction = {1.0F, 0.0F, 0.0F};

    CHECK(fixture_create(&fixture, transform(2.0F, 3.0F, 4.0F, 0.7F),
                         body(7.0F, 8.0F, 9.0F)));
    CHECK(fixture_state(&fixture, &before_spatial, &before_body));

#define CHECK_FAILURE(arguments)                                             \
    do {                                                                     \
        result = (HTHDynamicCollisionResult){true, true, true};               \
        CHECK(!hth_enemy_chase_apply arguments);                             \
        CHECK(result_is_empty(result));                                      \
        CHECK(state_unchanged(&fixture, before_spatial, before_body));        \
    } while (0)

    CHECK_FAILURE((NULL, fixture.actors, fixture.enemies, fixture.spatial,
                   fixture.bodies, &world, fixture.enemy, direction, 2.0F,
                   0.5F, &result));
    CHECK_FAILURE((fixture.entities, NULL, fixture.enemies, fixture.spatial,
                   fixture.bodies, &world, fixture.enemy, direction, 2.0F,
                   0.5F, &result));
    CHECK_FAILURE((fixture.entities, fixture.actors, NULL, fixture.spatial,
                   fixture.bodies, &world, fixture.enemy, direction, 2.0F,
                   0.5F, &result));
    CHECK_FAILURE((fixture.entities, fixture.actors, fixture.enemies, NULL,
                   fixture.bodies, &world, fixture.enemy, direction, 2.0F,
                   0.5F, &result));
    CHECK_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                   fixture.spatial, NULL, &world, fixture.enemy, direction,
                   2.0F, 0.5F, &result));
    CHECK_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                   fixture.spatial, fixture.bodies, NULL, fixture.enemy,
                   direction, 2.0F, 0.5F, &result));
    CHECK_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                   fixture.spatial, fixture.bodies, &world,
                   hth_entity_handle_invalid(), direction, 2.0F, 0.5F,
                   &result));
#undef CHECK_FAILURE

    CHECK(!chase(&fixture, &world, direction, 2.0F, 0.5F, NULL));
    CHECK(state_unchanged(&fixture, before_spatial, before_body));
    fixture_destroy(&fixture);
    return true;
}

static bool test_scalar_direction_and_product_validation(void)
{
    Fixture fixture;
    HTHCollisionWorld world = distant_world();
    const float invalid_scalars[] = {-1.0F, NAN, INFINITY, -INFINITY};
    const float invalid_components[] = {NAN, INFINITY, -INFINITY};
    HTHSpatialTransform before_spatial;
    HTHDynamicBody before_body;
    HTHDynamicCollisionResult result;
    HTHVec3 direction;
    size_t index;
    size_t component;

    CHECK(fixture_create(&fixture, transform(0.0F, 0.0F, 0.0F, -0.5F),
                         body(1.0F, 2.0F, 3.0F)));
    CHECK(fixture_state(&fixture, &before_spatial, &before_body));
    for (index = 0U;
         index < sizeof(invalid_scalars) / sizeof(invalid_scalars[0]);
         ++index) {
        result = (HTHDynamicCollisionResult){true, true, true};
        CHECK(!chase(&fixture, &world, (HTHVec3){1.0F, 0.0F, 0.0F},
                     invalid_scalars[index], 1.0F, &result));
        CHECK(result_is_empty(result));
        CHECK(state_unchanged(&fixture, before_spatial, before_body));
        result = (HTHDynamicCollisionResult){true, true, true};
        CHECK(!chase(&fixture, &world, (HTHVec3){1.0F, 0.0F, 0.0F},
                     1.0F, invalid_scalars[index], &result));
        CHECK(result_is_empty(result));
        CHECK(state_unchanged(&fixture, before_spatial, before_body));
    }
    for (component = 0U; component < 3U; ++component) {
        for (index = 0U;
             index < sizeof(invalid_components) /
                         sizeof(invalid_components[0]);
             ++index) {
            direction = (HTHVec3){1.0F, 1.0F, 1.0F};
            if (component == 0U) {
                direction.x = invalid_components[index];
            } else if (component == 1U) {
                direction.y = invalid_components[index];
            } else {
                direction.z = invalid_components[index];
            }
            result = (HTHDynamicCollisionResult){true, true, true};
            CHECK(!chase(&fixture, &world, direction, 1.0F, 1.0F,
                         &result));
            CHECK(result_is_empty(result));
            CHECK(state_unchanged(&fixture, before_spatial, before_body));
        }
    }
    result = (HTHDynamicCollisionResult){true, true, true};
    CHECK(!chase(&fixture, &world, (HTHVec3){FLT_MAX, 0.0F, 0.0F},
                 2.0F, 0.0F, &result));
    CHECK(result_is_empty(result));
    CHECK(state_unchanged(&fixture, before_spatial, before_body));
    fixture_destroy(&fixture);
    return true;
}

static bool test_association_remove_and_reattach(void)
{
    Fixture fixture;
    HTHCollisionWorld world = distant_world();
    HTHSpatialTransform spatial_value = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHDynamicBody body_value = body(3.0F, 2.0F, 1.0F);
    HTHDynamicCollisionResult result;
    HTHVec3 direction = {1.0F, 0.0F, 0.0F};

    CHECK(fixture_create(&fixture, spatial_value, body_value));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities,
                                 fixture.enemy));
    CHECK(!chase(&fixture, &world, direction, 1.0F, 0.0F, &result));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 fixture.enemy));
    CHECK(chase(&fixture, &world, direction, 1.0F, 0.0F, &result));

    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities,
                                 fixture.enemy));
    CHECK(!chase(&fixture, &world, direction, 1.0F, 0.0F, &result));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, fixture.enemy));
    CHECK(chase(&fixture, &world, direction, 1.0F, 0.0F, &result));

    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   fixture.enemy));
    CHECK(!chase(&fixture, &world, direction, 1.0F, 0.0F, &result));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   fixture.enemy, &spatial_value));
    CHECK(chase(&fixture, &world, direction, 1.0F, 0.0F, &result));

    CHECK(hth_dynamic_body_remove(fixture.bodies, fixture.entities,
                                  fixture.enemy));
    CHECK(!chase(&fixture, &world, direction, 1.0F, 0.0F, &result));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, fixture.enemy,
                                  &body_value));
    CHECK(chase(&fixture, &world, direction, 1.0F, 0.0F, &result));
    fixture_destroy(&fixture);
    return true;
}

static bool test_missing_roles_and_generation_safety(void)
{
    Fixture fixture;
    HTHCollisionWorld world = distant_world();
    HTHSpatialTransform spatial_value = transform(5.0F, 0.0F, 0.0F, 0.0F);
    HTHDynamicBody body_value = body(4.0F, 0.0F, 0.0F);
    HTHDynamicCollisionResult result;
    HTHEntityHandle entity_only;
    HTHEntityHandle actor_only;
    HTHEntityHandle enemy_without_spatial;
    HTHEntityHandle enemy_without_body;
    HTHEntityHandle stale;
    HTHEntityHandle replacement;
    HTHSpatialTransform replacement_after;
    HTHDynamicBody replacement_body;

    CHECK(fixture_create(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                         body(1.0F, 0.0F, 0.0F)));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &entity_only));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   entity_only, &spatial_value));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, entity_only, &body_value));
    CHECK(!hth_enemy_chase_apply(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.bodies, &world, entity_only, (HTHVec3){1.0F, 0.0F, 0.0F},
        1.0F, 1.0F, &result));

    CHECK(hth_entity_registry_create_entity(fixture.entities, &actor_only));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 actor_only));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   actor_only, &spatial_value));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, actor_only, &body_value));
    CHECK(!hth_enemy_chase_apply(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.bodies, &world, actor_only, (HTHVec3){1.0F, 0.0F, 0.0F},
        1.0F, 1.0F, &result));

    CHECK(hth_entity_registry_create_entity(fixture.entities,
                                            &enemy_without_spatial));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 enemy_without_spatial));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy_without_spatial));
    CHECK(!hth_enemy_chase_apply(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.bodies, &world, enemy_without_spatial,
        (HTHVec3){1.0F, 0.0F, 0.0F}, 1.0F, 1.0F, &result));

    CHECK(hth_entity_registry_create_entity(fixture.entities,
                                            &enemy_without_body));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 enemy_without_body));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy_without_body));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   enemy_without_body, &spatial_value));
    CHECK(!hth_enemy_chase_apply(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.bodies, &world, enemy_without_body,
        (HTHVec3){1.0F, 0.0F, 0.0F}, 1.0F, 1.0F, &result));

    stale = fixture.enemy;
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &replacement));
    CHECK(replacement.index == stale.index);
    CHECK(replacement.generation != stale.generation);
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 replacement));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, replacement));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   replacement, &spatial_value));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, replacement, &body_value));
    CHECK(!hth_enemy_chase_apply(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.bodies, &world, stale, (HTHVec3){1.0F, 0.0F, 0.0F}, 4.0F,
        1.0F, &result));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                replacement, &replacement_after));
    CHECK(close_vector(replacement_after.position,
                       spatial_value.position));
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities, replacement,
                               &replacement_body));
    CHECK(close_vector(replacement_body.velocity, body_value.velocity));
    fixture_destroy(&fixture);
    return true;
}

static bool test_zero_requests_and_zero_dt(void)
{
    Fixture fixture;
    HTHCollisionWorld world = distant_world();
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 1.3F);
    HTHDynamicBody initial = body(7.0F, 8.0F, 9.0F);
    HTHSpatialTransform after_spatial;
    HTHDynamicBody after_body;
    HTHDynamicCollisionResult result;

    CHECK(fixture_create(&fixture, origin, initial));
    CHECK(chase(&fixture, &world, (HTHVec3){0.0F, 0.0F, 0.0F}, 4.0F,
                1.0F, &result));
    CHECK(result_is_empty(result));
    CHECK(fixture_state(&fixture, &after_spatial, &after_body));
    CHECK(close_vector(after_spatial.position, origin.position));
    CHECK(close_vector(after_body.velocity,
                       (HTHVec3){0.0F, 0.0F, 0.0F}));

    CHECK(reset_state(&fixture, origin, initial));
    CHECK(chase(&fixture, &world, (HTHVec3){1.0F, 0.0F, 0.0F}, 0.0F,
                1.0F, &result));
    CHECK(result_is_empty(result));
    CHECK(fixture_state(&fixture, &after_spatial, &after_body));
    CHECK(close_vector(after_body.velocity,
                       (HTHVec3){0.0F, 0.0F, 0.0F}));

    CHECK(reset_state(&fixture, origin, initial));
    CHECK(chase(&fixture, &world, (HTHVec3){0.0F, 0.0F, 0.0F}, 0.0F,
                1.0F, &result));
    CHECK(result_is_empty(result));

    CHECK(reset_state(&fixture, origin, initial));
    CHECK(chase(&fixture, &world, (HTHVec3){1.0F, 0.0F, 0.0F}, 4.0F,
                0.0F, &result));
    CHECK(result_is_empty(result));
    CHECK(fixture_state(&fixture, &after_spatial, &after_body));
    CHECK(close_vector(after_spatial.position, origin.position));
    CHECK(close_vector(after_body.velocity,
                       (HTHVec3){4.0F, 0.0F, 0.0F}));
    CHECK(close_float(after_spatial.yaw, origin.yaw));
    fixture_destroy(&fixture);
    return true;
}

static bool test_axis_and_diagonal_movement_from_seek(void)
{
    Fixture fixture;
    HTHCollisionWorld world = distant_world();
    const HTHSpatialTransform targets[] = {
        {{10.0F, 0.0F, 0.0F}, 0.0F}, {{-10.0F, 0.0F, 0.0F}, 0.0F},
        {{0.0F, 10.0F, 0.0F}, 0.0F}, {{0.0F, -10.0F, 0.0F}, 0.0F},
        {{0.0F, 0.0F, 10.0F}, 0.0F}, {{0.0F, 0.0F, -10.0F}, 0.0F}
    };
    const HTHVec3 expected_positions[] = {
        {2.0F, 0.0F, 0.0F}, {-2.0F, 0.0F, 0.0F},
        {0.0F, 2.0F, 0.0F}, {0.0F, -2.0F, 0.0F},
        {0.0F, 0.0F, 2.0F}, {0.0F, 0.0F, -2.0F}
    };
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, -2.4F);
    HTHDynamicBody initial = body(9.0F, 8.0F, 7.0F);
    HTHEntityHandle target;
    HTHSpatialTransform after_spatial;
    HTHDynamicBody after_body;
    HTHDynamicCollisionResult result;
    HTHVec3 direction;
    size_t index;

    CHECK(fixture_create(&fixture, origin, initial));
    CHECK(create_spatial_entity(&fixture, targets[0], &target));
    for (index = 0U; index < sizeof(targets) / sizeof(targets[0]); ++index) {
        CHECK(reset_state(&fixture, origin, initial));
        CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, target,
                                    &targets[index]));
        CHECK(hth_enemy_seek_compute(
            fixture.entities, fixture.actors, fixture.enemies,
            fixture.spatial, fixture.enemy, target, &direction));
        CHECK(chase(&fixture, &world, direction, 4.0F, 0.5F, &result));
        CHECK(result.moved && !result.collided && !result.start_solid);
        CHECK(fixture_state(&fixture, &after_spatial, &after_body));
        CHECK(close_vector(after_spatial.position,
                           expected_positions[index]));
        CHECK(close_vector(after_body.velocity,
                           (HTHVec3){direction.x * 4.0F,
                                     direction.y * 4.0F,
                                     direction.z * 4.0F}));
        CHECK(close_float(after_spatial.yaw, origin.yaw));
    }

    CHECK(reset_state(&fixture, origin, initial));
    CHECK(hth_spatial_store_set(
        fixture.spatial, fixture.entities, target,
        &(HTHSpatialTransform){{3.0F, 4.0F, 0.0F}, 8.0F}));
    CHECK(hth_enemy_seek_compute(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.enemy, target, &direction));
    CHECK(close_vector(direction, (HTHVec3){0.6F, 0.8F, 0.0F}));
    CHECK(chase(&fixture, &world, direction, 5.0F, 1.0F, &result));
    CHECK(fixture_state(&fixture, &after_spatial, &after_body));
    CHECK(close_vector(after_spatial.position,
                       (HTHVec3){3.0F, 4.0F, 0.0F}));
    CHECK(close_vector(after_body.velocity,
                       (HTHVec3){3.0F, 4.0F, 0.0F}));
    CHECK(close_float(sqrtf(after_body.velocity.x * after_body.velocity.x +
                            after_body.velocity.y * after_body.velocity.y +
                            after_body.velocity.z * after_body.velocity.z),
                      5.0F));
    fixture_destroy(&fixture);
    return true;
}

static bool test_collision_sliding_and_start_solid(void)
{
    Fixture fixture;
    HTHCollisionWorld wall = wall_world();
    HTHCollisionWorld solid = start_solid_world();
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.8F);
    HTHDynamicBody initial = body(-1.0F, -2.0F, -3.0F);
    HTHSpatialTransform after_spatial;
    HTHDynamicBody after_body;
    HTHDynamicCollisionResult result;

    CHECK(fixture_create(&fixture, origin, initial));
    CHECK(chase(&fixture, &wall, (HTHVec3){1.0F, 0.0F, 0.0F}, 1000.0F,
                1.0F, &result));
    CHECK(result.moved && result.collided && !result.start_solid);
    CHECK(fixture_state(&fixture, &after_spatial, &after_body));
    CHECK(close_float(after_spatial.position.x, 1.5F));
    CHECK(close_vector(after_body.velocity,
                       (HTHVec3){0.0F, 0.0F, 0.0F}));

    CHECK(reset_state(&fixture, origin, initial));
    CHECK(chase(&fixture, &wall, (HTHVec3){0.8F, 0.0F, 0.6F}, 5.0F,
                1.0F, &result));
    CHECK(result.moved && result.collided && !result.start_solid);
    CHECK(fixture_state(&fixture, &after_spatial, &after_body));
    CHECK(close_vector(after_spatial.position,
                       (HTHVec3){1.5F, 0.0F, 3.0F}));
    CHECK(close_vector(after_body.velocity,
                       (HTHVec3){0.0F, 0.0F, 3.0F}));
    CHECK(close_float(after_spatial.yaw, origin.yaw));

    CHECK(reset_state(&fixture, origin, initial));
    CHECK(chase(&fixture, &solid, (HTHVec3){1.0F, 0.0F, 0.0F}, 3.0F,
                1.0F, &result));
    CHECK(!result.moved && result.collided && result.start_solid);
    CHECK(fixture_state(&fixture, &after_spatial, &after_body));
    CHECK(close_vector(after_spatial.position, origin.position));
    CHECK(close_vector(after_body.velocity,
                       (HTHVec3){3.0F, 0.0F, 0.0F}));
    CHECK(close_float(after_spatial.yaw, origin.yaw));
    fixture_destroy(&fixture);
    return true;
}

static bool test_downstream_failure_restores_velocity(void)
{
    Fixture fixture;
    HTHCollisionWorld world = distant_world();
    HTHSpatialTransform before_spatial;
    HTHDynamicBody before_body;
    HTHDynamicCollisionResult result = {true, true, true};

    CHECK(fixture_create(&fixture, transform(2.0F, 3.0F, 4.0F, 0.2F),
                         body(1.0F, 2.0F, 3.0F)));
    CHECK(fixture_state(&fixture, &before_spatial, &before_body));
    CHECK(!chase(&fixture, &world, (HTHVec3){FLT_MAX, 0.0F, 0.0F},
                 1.0F, 2.0F, &result));
    CHECK(result_is_empty(result));
    CHECK(state_unchanged(&fixture, before_spatial, before_body));
    fixture_destroy(&fixture);
    return true;
}

static bool test_health_independence_reversal_and_determinism(void)
{
    Fixture first;
    Fixture second;
    HTHCollisionWorld world = distant_world();
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, -1.7F);
    HTHDynamicBody initial = body(5.0F, 6.0F, 7.0F);
    HTHHealth zero_health = {0.0F, 100.0F};
    HTHSpatialTransform first_spatial;
    HTHSpatialTransform second_spatial;
    HTHDynamicBody first_body;
    HTHDynamicBody second_body;
    HTHDynamicCollisionResult first_result;
    HTHDynamicCollisionResult second_result;
    size_t index;

    CHECK(fixture_create(&first, origin, initial));
    CHECK(fixture_create(&second, origin, initial));
    CHECK(hth_entity_handle_equal(first.enemy, second.enemy));
    CHECK(chase(&first, &world, (HTHVec3){1.0F, 0.0F, 0.0F}, 2.0F,
                1.0F, &first_result));
    CHECK(reset_state(&first, origin, initial));
    CHECK(chase(&first, &world, (HTHVec3){-1.0F, 0.0F, 0.0F}, 2.0F,
                1.0F, &first_result));
    CHECK(fixture_state(&first, &first_spatial, &first_body));
    CHECK(close_vector(first_spatial.position,
                       (HTHVec3){-2.0F, 0.0F, 0.0F}));

    CHECK(hth_health_store_attach(first.health, first.entities, first.actors,
                                  first.enemy, zero_health));
    CHECK(reset_state(&first, origin, initial));
    CHECK(chase(&first, &world, (HTHVec3){0.0F, 1.0F, 0.0F}, 2.0F,
                1.0F, &first_result));
    CHECK(fixture_state(&first, &first_spatial, &first_body));
    CHECK(close_vector(first_spatial.position,
                       (HTHVec3){0.0F, 2.0F, 0.0F}));

    for (index = 0U; index < 128U; ++index) {
        CHECK(reset_state(&first, origin, initial));
        CHECK(reset_state(&second, origin, initial));
        CHECK(chase(&first, &world, (HTHVec3){0.0F, 0.0F, 1.0F}, 3.0F,
                    0.5F, &first_result));
        CHECK(chase(&second, &world, (HTHVec3){0.0F, 0.0F, 1.0F}, 3.0F,
                    0.5F, &second_result));
        CHECK(fixture_state(&first, &first_spatial, &first_body));
        CHECK(fixture_state(&second, &second_spatial, &second_body));
        CHECK(memcmp(&first_result, &second_result,
                     sizeof(first_result)) == 0);
        CHECK(memcmp(&first_spatial, &second_spatial,
                     sizeof(first_spatial)) == 0);
        CHECK(memcmp(&first_body, &second_body, sizeof(first_body)) == 0);
    }
    fixture_destroy(&second);
    fixture_destroy(&first);
    return true;
}

static bool test_composition_and_idle_caller_separation(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = distant_world();
    HTHCollisionWorld blocked = wall_world();
    HTHEntityHandle target;
    HTHEntityHandle farther;
    HTHEntityHandle candidates[2];
    HTHEntityHandle selected;
    HTHEnemyIntent intent;
    HTHVec3 direction;
    HTHDynamicCollisionResult result;
    HTHSpatialTransform before;
    HTHSpatialTransform after;

    CHECK(fixture_create(&fixture, transform(0.0F, 0.0F, 0.0F, 0.4F),
                         body(0.0F, 0.0F, 0.0F)));
    CHECK(create_spatial_entity(
        &fixture, transform(3.0F, 4.0F, 0.0F, 0.0F), &target));
    CHECK(create_spatial_entity(
        &fixture, transform(0.0F, 9.0F, 0.0F, 0.0F), &farther));

    CHECK(hth_enemy_decision_evaluate(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &clear, fixture.enemy, 10.0F, &intent));
    CHECK(intent.kind == HTH_ENEMY_INTENT_IDLE);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                fixture.enemy, &before));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                fixture.enemy, &after));
    CHECK(memcmp(&before, &after, sizeof(before)) == 0);

    CHECK(hth_enemy_target_store_set(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        fixture.enemy, target));
    CHECK(hth_enemy_decision_evaluate(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &clear, fixture.enemy, 10.0F, &intent));
    CHECK(intent.kind == HTH_ENEMY_INTENT_PURSUE);
    CHECK(hth_enemy_seek_compute(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.enemy, intent.target, &direction));
    CHECK(chase(&fixture, &clear, direction, 5.0F, 0.5F, &result));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                fixture.enemy, &after));
    CHECK(close_vector(after.position, (HTHVec3){1.5F, 2.0F, 0.0F}));

    CHECK(reset_state(&fixture, before, body(0.0F, 0.0F, 0.0F)));
    candidates[0] = farther;
    candidates[1] = target;
    CHECK(hth_enemy_target_select(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        &clear, fixture.targets, fixture.enemy, candidates, 2U,
        hth_entity_handle_invalid(), 10.0F, &selected));
    CHECK(hth_entity_handle_equal(selected, target));
    CHECK(hth_enemy_decision_evaluate(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &clear, fixture.enemy, 10.0F, &intent));
    CHECK(intent.kind == HTH_ENEMY_INTENT_PURSUE);
    CHECK(hth_enemy_seek_compute(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.enemy, intent.target, &direction));
    CHECK(chase(&fixture, &clear, direction, 5.0F, 0.5F, &result));

    CHECK(reset_state(&fixture, before, body(0.0F, 0.0F, 0.0F)));
    CHECK(hth_enemy_decision_evaluate(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &blocked, fixture.enemy, 10.0F, &intent));
    CHECK(intent.kind == HTH_ENEMY_INTENT_IDLE);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                fixture.enemy, &after));
    CHECK(memcmp(&before, &after, sizeof(before)) == 0);

    CHECK(hth_entity_registry_destroy_entity(fixture.entities, target));
    CHECK(hth_enemy_decision_evaluate(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &clear, fixture.enemy, 10.0F, &intent));
    CHECK(intent.kind == HTH_ENEMY_INTENT_IDLE);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                fixture.enemy, &after));
    CHECK(memcmp(&before, &after, sizeof(before)) == 0);
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_argument_failures_are_transactional,
        test_scalar_direction_and_product_validation,
        test_association_remove_and_reattach,
        test_missing_roles_and_generation_safety,
        test_zero_requests_and_zero_dt,
        test_axis_and_diagonal_movement_from_seek,
        test_collision_sliding_and_start_solid,
        test_downstream_failure_restores_velocity,
        test_health_independence_reversal_and_determinism,
        test_composition_and_idle_caller_separation
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("enemy chase tests passed");
    return EXIT_SUCCESS;
}
