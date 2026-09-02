#include "enemy_pursuit_runtime.h"

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
    HTHSpatialStore *spatial;
    HTHDynamicBodyStore *bodies;
    HTHEnemyTargetStore *targets;
} Context;

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

static HTHCollisionWorld los_blocking_world(void)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] = (HTHAABB){{2.0F, -10.0F, -10.0F},
                                   {2.2F, 10.0F, 10.0F}};
    world.obstacle_count = 1U;
    return world;
}

static HTHCollisionWorld body_only_blocking_world(void)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] = (HTHAABB){{2.0F, 0.4F, -10.0F},
                                   {2.2F, 2.0F, 10.0F}};
    world.obstacle_count = 1U;
    return world;
}

static bool context_create(Context *context)
{
    *context = (Context){0};
    context->entities = hth_entity_registry_create();
    context->actors = hth_actor_store_create();
    context->enemies = hth_enemy_store_create();
    context->spatial = hth_spatial_store_create();
    context->bodies = hth_dynamic_body_store_create();
    context->targets = hth_enemy_target_store_create();
    return context->entities != NULL && context->actors != NULL &&
           context->enemies != NULL && context->spatial != NULL &&
           context->bodies != NULL && context->targets != NULL;
}

static void context_destroy(Context *context)
{
    hth_enemy_target_store_destroy(context->targets);
    hth_dynamic_body_store_destroy(context->bodies);
    hth_spatial_store_destroy(context->spatial);
    hth_enemy_store_destroy(context->enemies);
    hth_actor_store_destroy(context->actors);
    hth_entity_registry_destroy(context->entities);
}

static bool create_spatial_entity(Context *context,
                                  HTHSpatialTransform spatial_value,
                                  HTHEntityHandle *out_entity)
{
    return hth_entity_registry_create_entity(context->entities, out_entity) &&
           hth_spatial_store_attach(context->spatial, context->entities,
                                    *out_entity, &spatial_value);
}

static bool create_enemy(Context *context,
                         const HTHSpatialTransform *spatial_value,
                         const HTHDynamicBody *body_value,
                         HTHEntityHandle *out_enemy)
{
    if (!hth_entity_registry_create_entity(context->entities, out_enemy) ||
        !hth_actor_store_attach(context->actors, context->entities,
                                *out_enemy) ||
        !hth_enemy_store_attach(context->enemies, context->entities,
                                context->actors, *out_enemy)) {
        return false;
    }
    if (spatial_value == NULL) {
        return body_value == NULL;
    }
    if (!hth_spatial_store_attach(context->spatial, context->entities,
                                  *out_enemy, spatial_value)) {
        return false;
    }
    return body_value == NULL ||
           hth_dynamic_body_attach(context->bodies, context->entities,
                                   context->spatial, *out_enemy, body_value);
}

static bool step(Context *context, const HTHCollisionWorld *world,
                 const HTHEntityHandle *candidates, size_t candidate_count,
                 float radius, float speed, float dt)
{
    return hth_enemy_pursuit_runtime_step(
        context->entities, context->actors, context->enemies,
        context->spatial, context->bodies, context->targets, world,
        candidates, candidate_count, radius, speed, dt);
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

static bool get_state(Context *context, HTHEntityHandle enemy,
                      HTHSpatialTransform *out_spatial,
                      HTHDynamicBody *out_body)
{
    return hth_spatial_store_get(context->spatial, context->entities, enemy,
                                 out_spatial) &&
           hth_dynamic_body_get(context->bodies, context->entities, enemy,
                                out_body);
}

static bool target_equals(Context *context, HTHEntityHandle enemy,
                          HTHEntityHandle expected)
{
    HTHEntityHandle actual;

    return hth_enemy_target_store_get(
               context->targets, context->entities, context->actors,
               context->enemies, enemy, &actual) &&
           hth_entity_handle_equal(actual, expected);
}

static bool no_target(Context *context, HTHEntityHandle enemy)
{
    HTHEntityHandle target;

    return !hth_enemy_target_store_get(
        context->targets, context->entities, context->actors,
        context->enemies, enemy, &target);
}

static bool test_global_validation_is_transactional(void)
{
    Context context;
    HTHCollisionWorld world = distant_world();
    HTHSpatialTransform initial_spatial =
        transform(0.0F, 0.0F, 0.0F, 0.6F);
    HTHDynamicBody initial_body = body(1.0F, 2.0F, 3.0F);
    HTHSpatialTransform after_spatial;
    HTHDynamicBody after_body;
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;
    HTHEntityHandle candidates[1];
    const float invalid[] = {-1.0F, NAN, INFINITY, -INFINITY};
    size_t index;

    CHECK(context_create(&context));
    CHECK(create_enemy(&context, &initial_spatial, &initial_body, &enemy));
    CHECK(create_spatial_entity(
        &context, transform(3.0F, 0.0F, 0.0F, 0.0F), &candidate));
    candidates[0] = candidate;

#define CHECK_UNCHANGED()                                                    \
    do {                                                                     \
        CHECK(no_target(&context, enemy));                                   \
        CHECK(get_state(&context, enemy, &after_spatial, &after_body));       \
        CHECK(memcmp(&initial_spatial, &after_spatial,                        \
                     sizeof(initial_spatial)) == 0);                          \
        CHECK(memcmp(&initial_body, &after_body, sizeof(initial_body)) == 0); \
    } while (0)

#define CHECK_FAILURE(arguments)                                             \
    do {                                                                     \
        CHECK(!hth_enemy_pursuit_runtime_step arguments);                    \
        CHECK_UNCHANGED();                                                   \
    } while (0)

    CHECK_FAILURE((NULL, context.actors, context.enemies, context.spatial,
                   context.bodies, context.targets, &world, candidates, 1U,
                   10.0F, 2.0F, 1.0F));
    CHECK_FAILURE((context.entities, NULL, context.enemies, context.spatial,
                   context.bodies, context.targets, &world, candidates, 1U,
                   10.0F, 2.0F, 1.0F));
    CHECK_FAILURE((context.entities, context.actors, NULL, context.spatial,
                   context.bodies, context.targets, &world, candidates, 1U,
                   10.0F, 2.0F, 1.0F));
    CHECK_FAILURE((context.entities, context.actors, context.enemies, NULL,
                   context.bodies, context.targets, &world, candidates, 1U,
                   10.0F, 2.0F, 1.0F));
    CHECK_FAILURE((context.entities, context.actors, context.enemies,
                   context.spatial, NULL, context.targets, &world, candidates,
                   1U, 10.0F, 2.0F, 1.0F));
    CHECK_FAILURE((context.entities, context.actors, context.enemies,
                   context.spatial, context.bodies, NULL, &world, candidates,
                   1U, 10.0F, 2.0F, 1.0F));
    CHECK_FAILURE((context.entities, context.actors, context.enemies,
                   context.spatial, context.bodies, context.targets, NULL,
                   candidates, 1U, 10.0F, 2.0F, 1.0F));
    CHECK_FAILURE((context.entities, context.actors, context.enemies,
                   context.spatial, context.bodies, context.targets, &world,
                   NULL, 1U, 10.0F, 2.0F, 1.0F));
    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        CHECK_FAILURE((context.entities, context.actors, context.enemies,
                       context.spatial, context.bodies, context.targets,
                       &world, candidates, 1U, invalid[index], 2.0F, 1.0F));
        CHECK_FAILURE((context.entities, context.actors, context.enemies,
                       context.spatial, context.bodies, context.targets,
                       &world, candidates, 1U, 10.0F, invalid[index], 1.0F));
        CHECK_FAILURE((context.entities, context.actors, context.enemies,
                       context.spatial, context.bodies, context.targets,
                       &world, candidates, 1U, 10.0F, 2.0F,
                       invalid[index]));
    }
#undef CHECK_FAILURE
#undef CHECK_UNCHANGED

    CHECK(step(&context, &world, NULL, 0U, 10.0F, 2.0F, 1.0F));
    CHECK(no_target(&context, enemy));
    context_destroy(&context);
    return true;
}

static bool test_empty_and_basic_runtime(void)
{
    Context context;
    HTHCollisionWorld world = distant_world();
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, -0.8F);
    HTHDynamicBody initial_body = body(0.0F, 0.0F, 0.0F);
    HTHSpatialTransform after_spatial;
    HTHDynamicBody after_body;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle candidates[1];

    CHECK(context_create(&context));
    CHECK(step(&context, &world, NULL, 0U, 10.0F, 5.0F, 1.0F));
    CHECK(create_enemy(&context, &origin, &initial_body, &enemy));
    CHECK(step(&context, &world, NULL, 0U, 10.0F, 5.0F, 1.0F));
    CHECK(no_target(&context, enemy));
    CHECK(get_state(&context, enemy, &after_spatial, &after_body));
    CHECK(close_vector(after_spatial.position, origin.position));

    CHECK(create_spatial_entity(
        &context, transform(3.0F, 4.0F, 0.0F, 0.0F), &target));
    candidates[0] = target;
    CHECK(step(&context, &world, candidates, 1U, 5.0F, 5.0F, 0.5F));
    CHECK(target_equals(&context, enemy, target));
    CHECK(get_state(&context, enemy, &after_spatial, &after_body));
    CHECK(close_vector(after_spatial.position,
                       (HTHVec3){1.5F, 2.0F, 0.0F}));
    CHECK(close_vector(after_body.velocity,
                       (HTHVec3){3.0F, 4.0F, 0.0F}));
    CHECK(close_float(after_spatial.yaw, origin.yaw));
    CHECK(hth_entity_handle_equal(candidates[0], target));
    context_destroy(&context);
    return true;
}

static bool test_candidate_filtering_and_consecutive_inputs(void)
{
    Context context;
    HTHCollisionWorld clear = distant_world();
    HTHCollisionWorld blocked = los_blocking_world();
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHDynamicBody initial_body = body(0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle outside;
    HTHEntityHandle stale;
    HTHEntityHandle valid;
    HTHEntityHandle list[5];
    HTHSpatialTransform after_spatial;
    HTHDynamicBody after_body;

    CHECK(context_create(&context));
    CHECK(create_enemy(&context, &origin, &initial_body, &enemy));
    CHECK(create_spatial_entity(
        &context, transform(20.0F, 0.0F, 0.0F, 0.0F), &outside));
    CHECK(create_spatial_entity(
        &context, transform(3.0F, 0.0F, 0.0F, 0.0F), &stale));
    CHECK(hth_entity_registry_destroy_entity(context.entities, stale));
    list[0] = enemy;
    list[1] = stale;
    list[2] = outside;
    CHECK(step(&context, &clear, list, 3U, 5.0F, 2.0F, 1.0F));
    CHECK(no_target(&context, enemy));
    CHECK(get_state(&context, enemy, &after_spatial, &after_body));
    CHECK(close_vector(after_spatial.position, origin.position));

    CHECK(create_spatial_entity(
        &context, transform(3.0F, 0.0F, 0.0F, 0.0F), &valid));
    list[0] = enemy;
    list[1] = stale;
    list[2] = valid;
    list[3] = valid;
    list[4] = outside;
    CHECK(step(&context, &blocked, list, 5U, 5.0F, 2.0F, 1.0F));
    CHECK(no_target(&context, enemy));
    CHECK(step(&context, &clear, list, 5U, 5.0F, 2.0F, 1.0F));
    CHECK(target_equals(&context, enemy, valid));
    CHECK(hth_entity_handle_equal(list[0], enemy));
    CHECK(hth_entity_handle_equal(list[1], stale));
    CHECK(hth_entity_handle_equal(list[2], valid));
    CHECK(hth_entity_handle_equal(list[3], valid));
    CHECK(hth_entity_handle_equal(list[4], outside));
    context_destroy(&context);
    return true;
}

static bool test_target_persistence_and_recovery(void)
{
    Context context;
    HTHCollisionWorld clear = distant_world();
    HTHCollisionWorld blocked = los_blocking_world();
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.3F);
    HTHSpatialTransform far_value = transform(8.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform outside_value =
        transform(30.0F, 0.0F, 0.0F, 0.0F);
    HTHDynamicBody initial_body = body(0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle current;
    HTHEntityHandle closer;
    HTHEntityHandle candidates[1];
    HTHSpatialTransform before;
    HTHSpatialTransform after;
    HTHDynamicBody after_body;

    CHECK(context_create(&context));
    CHECK(create_enemy(&context, &origin, &initial_body, &enemy));
    CHECK(create_spatial_entity(&context, far_value, &current));
    CHECK(create_spatial_entity(
        &context, transform(1.0F, 0.0F, 0.0F, 0.0F), &closer));
    CHECK(hth_enemy_target_store_set(
        context.targets, context.entities, context.actors, context.enemies,
        enemy, current));
    candidates[0] = closer;
    CHECK(step(&context, &clear, candidates, 1U, 10.0F, 2.0F, 0.5F));
    CHECK(target_equals(&context, enemy, current));
    CHECK(get_state(&context, enemy, &before, &after_body));
    CHECK(close_vector(before.position, (HTHVec3){1.0F, 0.0F, 0.0F}));

    CHECK(hth_spatial_store_set(context.spatial, context.entities, current,
                                &outside_value));
    CHECK(step(&context, &clear, candidates, 1U, 10.0F, 2.0F, 1.0F));
    CHECK(target_equals(&context, enemy, current));
    CHECK(hth_spatial_store_get(context.spatial, context.entities, enemy,
                                &after));
    CHECK(memcmp(&before, &after, sizeof(before)) == 0);

    CHECK(hth_spatial_store_set(context.spatial, context.entities, current,
                                &far_value));
    CHECK(step(&context, &blocked, candidates, 1U, 10.0F, 2.0F, 1.0F));
    CHECK(target_equals(&context, enemy, current));
    CHECK(hth_spatial_store_get(context.spatial, context.entities, enemy,
                                &after));
    CHECK(memcmp(&before, &after, sizeof(before)) == 0);

    CHECK(step(&context, &clear, candidates, 1U, 10.0F, 2.0F, 0.5F));
    CHECK(target_equals(&context, enemy, current));
    CHECK(hth_spatial_store_get(context.spatial, context.entities, enemy,
                                &after));
    CHECK(close_vector(after.position, (HTHVec3){2.0F, 0.0F, 0.0F}));
    context_destroy(&context);
    return true;
}

static bool test_stale_target_replacement_and_no_clear(void)
{
    Context context;
    HTHCollisionWorld world = distant_world();
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHDynamicBody initial_body = body(0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle stale;
    HTHEntityHandle replacement;
    HTHEntityHandle candidates[1];
    HTHSpatialTransform before;
    HTHSpatialTransform after;
    HTHDynamicBody after_body;

    CHECK(context_create(&context));
    CHECK(create_enemy(&context, &origin, &initial_body, &enemy));
    CHECK(create_spatial_entity(
        &context, transform(3.0F, 0.0F, 0.0F, 0.0F), &stale));
    CHECK(hth_enemy_target_store_set(
        context.targets, context.entities, context.actors, context.enemies,
        enemy, stale));
    CHECK(hth_entity_registry_destroy_entity(context.entities, stale));
    CHECK(get_state(&context, enemy, &before, &after_body));
    CHECK(step(&context, &world, NULL, 0U, 10.0F, 2.0F, 1.0F));
    CHECK(no_target(&context, enemy));
    CHECK(hth_spatial_store_get(context.spatial, context.entities, enemy,
                                &after));
    CHECK(memcmp(&before, &after, sizeof(before)) == 0);

    CHECK(create_spatial_entity(
        &context, transform(4.0F, 0.0F, 0.0F, 0.0F), &replacement));
    CHECK(replacement.index == stale.index);
    CHECK(replacement.generation != stale.generation);
    CHECK(no_target(&context, enemy));
    candidates[0] = replacement;
    CHECK(step(&context, &world, candidates, 1U, 10.0F, 2.0F, 1.0F));
    CHECK(target_equals(&context, enemy, replacement));
    CHECK(!target_equals(&context, enemy, stale));
    CHECK(hth_spatial_store_get(context.spatial, context.entities, enemy,
                                &after));
    CHECK(close_vector(after.position, (HTHVec3){2.0F, 0.0F, 0.0F}));
    context_destroy(&context);
    return true;
}

static bool test_multiple_enemies_and_capability_skips(void)
{
    Context context;
    HTHCollisionWorld world = distant_world();
    HTHSpatialTransform first_value = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform second_value = transform(20.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform bodyless_value =
        transform(40.0F, 0.0F, 0.0F, 0.0F);
    HTHDynamicBody initial_body = body(0.0F, 0.0F, 0.0F);
    HTHEntityHandle first;
    HTHEntityHandle no_spatial;
    HTHEntityHandle bodyless;
    HTHEntityHandle second;
    HTHEntityHandle first_target;
    HTHEntityHandle second_target;
    HTHEntityHandle bodyless_target;
    HTHEntityHandle candidates[3];
    HTHSpatialTransform after;
    HTHDynamicBody after_body;

    CHECK(context_create(&context));
    CHECK(create_enemy(&context, &first_value, &initial_body, &first));
    CHECK(create_enemy(&context, NULL, NULL, &no_spatial));
    CHECK(create_enemy(&context, &bodyless_value, NULL, &bodyless));
    CHECK(create_enemy(&context, &second_value, &initial_body, &second));
    CHECK(first.index < no_spatial.index &&
          no_spatial.index < bodyless.index && bodyless.index < second.index);
    CHECK(create_spatial_entity(
        &context, transform(2.0F, 0.0F, 0.0F, 0.0F), &first_target));
    CHECK(create_spatial_entity(
        &context, transform(22.0F, 0.0F, 0.0F, 0.0F), &second_target));
    CHECK(create_spatial_entity(
        &context, transform(42.0F, 0.0F, 0.0F, 0.0F), &bodyless_target));
    candidates[0] = second_target;
    candidates[1] = bodyless_target;
    candidates[2] = first_target;
    CHECK(step(&context, &world, candidates, 3U, 5.0F, 1.0F, 1.0F));
    CHECK(target_equals(&context, first, first_target));
    CHECK(target_equals(&context, second, second_target));
    CHECK(target_equals(&context, bodyless, bodyless_target));
    CHECK(no_target(&context, no_spatial));
    CHECK(get_state(&context, first, &after, &after_body));
    CHECK(close_vector(after.position, (HTHVec3){1.0F, 0.0F, 0.0F}));
    CHECK(get_state(&context, second, &after, &after_body));
    CHECK(close_vector(after.position, (HTHVec3){21.0F, 0.0F, 0.0F}));
    CHECK(hth_spatial_store_get(context.spatial, context.entities, bodyless,
                                &after));
    CHECK(close_vector(after.position, bodyless_value.position));

    CHECK(hth_dynamic_body_attach(context.bodies, context.entities,
                                  context.spatial, bodyless, &initial_body));
    candidates[0] = first_target;
    CHECK(step(&context, &world, candidates, 1U, 5.0F, 1.0F, 1.0F));
    CHECK(target_equals(&context, bodyless, bodyless_target));
    CHECK(hth_spatial_store_get(context.spatial, context.entities, bodyless,
                                &after));
    CHECK(close_vector(after.position, (HTHVec3){41.0F, 0.0F, 0.0F}));
    context_destroy(&context);
    return true;
}

static bool test_shared_candidate_zero_speed_dt_and_colocation(void)
{
    Context context;
    HTHCollisionWorld world = distant_world();
    HTHDynamicBody initial_body = body(5.0F, 6.0F, 7.0F);
    HTHSpatialTransform first_value = transform(-2.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform second_value = transform(2.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle first;
    HTHEntityHandle second;
    HTHEntityHandle shared;
    HTHEntityHandle candidates[1];
    HTHSpatialTransform before;
    HTHSpatialTransform after;
    HTHDynamicBody after_body;

    CHECK(context_create(&context));
    CHECK(create_enemy(&context, &first_value, &initial_body, &first));
    CHECK(create_enemy(&context, &second_value, &initial_body, &second));
    CHECK(create_enemy(
        &context, &(HTHSpatialTransform){{0.0F, 0.0F, 0.0F}, 0.0F},
        NULL, &shared));
    candidates[0] = shared;
    CHECK(step(&context, &world, candidates, 1U, 5.0F, 0.0F, 1.0F));
    CHECK(target_equals(&context, first, shared));
    CHECK(target_equals(&context, second, shared));
    CHECK(get_state(&context, first, &after, &after_body));
    CHECK(close_vector(after.position, first_value.position));
    CHECK(close_vector(after_body.velocity,
                       (HTHVec3){0.0F, 0.0F, 0.0F}));

    CHECK(get_state(&context, second, &before, &after_body));
    CHECK(step(&context, &world, candidates, 1U, 5.0F, 2.0F, 0.0F));
    CHECK(get_state(&context, second, &after, &after_body));
    CHECK(memcmp(&before, &after, sizeof(before)) == 0);

    CHECK(hth_spatial_store_set(
        context.spatial, context.entities, first,
        &(HTHSpatialTransform){{0.0F, 0.0F, 0.0F}, 1.2F}));
    CHECK(step(&context, &world, candidates, 1U, 5.0F, 2.0F, 1.0F));
    CHECK(step(&context, &world, candidates, 1U, 5.0F, 2.0F, 1.0F));
    CHECK(target_equals(&context, first, shared));
    CHECK(get_state(&context, first, &after, &after_body));
    CHECK(close_vector(after.position, (HTHVec3){0.0F, 0.0F, 0.0F}));
    CHECK(close_vector(after_body.velocity,
                       (HTHVec3){0.0F, 0.0F, 0.0F}));
    context_destroy(&context);
    return true;
}

static bool test_los_clear_body_block_and_slide(void)
{
    Context context;
    HTHCollisionWorld world = body_only_blocking_world();
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.9F);
    HTHDynamicBody initial_body = body(0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle candidates[1];
    HTHSpatialTransform after;
    HTHDynamicBody after_body;

    CHECK(context_create(&context));
    CHECK(create_enemy(&context, &origin, &initial_body, &enemy));
    CHECK(create_spatial_entity(
        &context, transform(4.0F, 0.0F, 3.0F, 0.0F), &target));
    candidates[0] = target;
    CHECK(step(&context, &world, candidates, 1U, 5.0F, 5.0F, 1.0F));
    CHECK(target_equals(&context, enemy, target));
    CHECK(get_state(&context, enemy, &after, &after_body));
    CHECK(close_vector(after.position, (HTHVec3){1.5F, 0.0F, 3.0F}));
    CHECK(close_vector(after_body.velocity,
                       (HTHVec3){0.0F, 0.0F, 3.0F}));
    CHECK(close_float(after.yaw, origin.yaw));
    context_destroy(&context);
    return true;
}

static bool build_failure_context(Context *context,
                                  HTHEntityHandle *out_first,
                                  HTHEntityHandle *out_second,
                                  HTHEntityHandle *out_third,
                                  HTHEntityHandle *out_candidate)
{
    HTHSpatialTransform first_value = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform second_value = transform(10.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform third_value = transform(20.0F, 0.0F, 0.0F, 0.0F);
    HTHDynamicBody initial_body = body(1.0F, 2.0F, 3.0F);

    return context_create(context) &&
           create_enemy(context, &first_value, &initial_body, out_first) &&
           create_enemy(context, &second_value, &initial_body, out_second) &&
           create_enemy(context, &third_value, &initial_body, out_third) &&
           create_spatial_entity(context, first_value, out_candidate);
}

static bool test_partial_progress_and_deterministic_failure(void)
{
    Context first_context;
    Context second_context;
    HTHCollisionWorld world = distant_world();
    HTHEntityHandle first;
    HTHEntityHandle second;
    HTHEntityHandle third;
    HTHEntityHandle candidate;
    HTHEntityHandle first_b;
    HTHEntityHandle second_b;
    HTHEntityHandle third_b;
    HTHEntityHandle candidate_b;
    HTHEntityHandle candidates[1];
    HTHEntityHandle candidates_b[1];
    HTHSpatialTransform second_before;
    HTHSpatialTransform second_after;
    HTHDynamicBody second_body_before;
    HTHDynamicBody second_body_after;

    CHECK(build_failure_context(&first_context, &first, &second, &third,
                                &candidate));
    candidates[0] = candidate;
    CHECK(get_state(&first_context, second, &second_before,
                    &second_body_before));
    CHECK(!step(&first_context, &world, candidates, 1U, 30.0F, FLT_MAX,
                2.0F));
    CHECK(target_equals(&first_context, first, candidate));
    CHECK(target_equals(&first_context, second, candidate));
    CHECK(no_target(&first_context, third));
    CHECK(get_state(&first_context, second, &second_after,
                    &second_body_after));
    CHECK(memcmp(&second_before, &second_after,
                 sizeof(second_before)) == 0);
    CHECK(memcmp(&second_body_before, &second_body_after,
                 sizeof(second_body_before)) == 0);

    CHECK(build_failure_context(&second_context, &first_b, &second_b,
                                &third_b, &candidate_b));
    candidates_b[0] = candidate_b;
    CHECK(!step(&second_context, &world, candidates_b, 1U, 30.0F, FLT_MAX,
                2.0F));
    CHECK(target_equals(&second_context, first_b, candidate_b));
    CHECK(target_equals(&second_context, second_b, candidate_b));
    CHECK(no_target(&second_context, third_b));
    context_destroy(&second_context);
    context_destroy(&first_context);
    return true;
}

static bool test_first_enemy_failure_and_independent_contexts(void)
{
    Context failing;
    Context untouched;
    HTHCollisionWorld world = distant_world();
    HTHSpatialTransform origin = transform(10.0F, 0.0F, 0.0F, 0.0F);
    HTHDynamicBody initial_body = body(1.0F, 0.0F, 0.0F);
    HTHEntityHandle failing_enemy;
    HTHEntityHandle failing_target;
    HTHEntityHandle untouched_enemy;
    HTHEntityHandle untouched_target;
    HTHEntityHandle candidates[1];
    HTHSpatialTransform untouched_before;
    HTHSpatialTransform untouched_after;
    HTHDynamicBody untouched_body_before;
    HTHDynamicBody untouched_body_after;

    CHECK(context_create(&failing));
    CHECK(context_create(&untouched));
    CHECK(create_enemy(&failing, &origin, &initial_body, &failing_enemy));
    CHECK(create_spatial_entity(
        &failing, transform(0.0F, 0.0F, 0.0F, 0.0F), &failing_target));
    CHECK(create_enemy(&untouched, &origin, &initial_body, &untouched_enemy));
    CHECK(create_spatial_entity(
        &untouched, transform(0.0F, 0.0F, 0.0F, 0.0F),
        &untouched_target));
    CHECK(hth_entity_handle_equal(failing_enemy, untouched_enemy));
    CHECK(hth_entity_handle_equal(failing_target, untouched_target));
    CHECK(get_state(&untouched, untouched_enemy, &untouched_before,
                    &untouched_body_before));
    candidates[0] = failing_target;
    CHECK(!step(&failing, &world, candidates, 1U, 20.0F, FLT_MAX, 2.0F));
    CHECK(get_state(&untouched, untouched_enemy, &untouched_after,
                    &untouched_body_after));
    CHECK(memcmp(&untouched_before, &untouched_after,
                 sizeof(untouched_before)) == 0);
    CHECK(memcmp(&untouched_body_before, &untouched_body_after,
                 sizeof(untouched_body_before)) == 0);
    CHECK(no_target(&untouched, untouched_enemy));
    context_destroy(&untouched);
    context_destroy(&failing);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_global_validation_is_transactional,
        test_empty_and_basic_runtime,
        test_candidate_filtering_and_consecutive_inputs,
        test_target_persistence_and_recovery,
        test_stale_target_replacement_and_no_clear,
        test_multiple_enemies_and_capability_skips,
        test_shared_candidate_zero_speed_dt_and_colocation,
        test_los_clear_body_block_and_slide,
        test_partial_progress_and_deterministic_failure,
        test_first_enemy_failure_and_independent_contexts
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("enemy pursuit runtime tests passed");
    return EXIT_SUCCESS;
}
