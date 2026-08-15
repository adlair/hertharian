#include "collision_trace.h"
#include "collision_world.h"
#include "player_body.h"
#include "world.h"

#include <assert.h>
#include <math.h>

static bool close_enough(float left, float right)
{
    return fabsf(left - right) <= 1.0e-6F;
}

static void assert_vec3(HTHVec3 actual, HTHVec3 expected)
{
    assert(close_enough(actual.x, expected.x));
    assert(close_enough(actual.y, expected.y));
    assert(close_enough(actual.z, expected.z));
}

static void test_empty_world_lifecycle(void)
{
    HTHWorld world;
    HTHAABB bounds;
    HTHWorldSpawn spawn;

    assert(hth_world_init(&world));
    assert(!hth_world_is_finalized(&world));
    assert(hth_world_static_object_count(&world) == 0U);
    assert(hth_world_static_object(&world, 0U) == NULL);
    assert(!hth_world_bounds(&world, &bounds));
    assert(!hth_world_default_spawn(&world, &spawn));
    assert(hth_world_finalize(&world));
    assert(hth_world_is_finalized(&world));
    assert(hth_world_static_object_count(&world) == 0U);
    assert(!hth_world_bounds(&world, &bounds));
    assert(!hth_world_default_spawn(&world, &spawn));
    assert(!hth_world_finalize(&world));
    hth_world_shutdown(&world);
}

static void test_validation_and_immutability(void)
{
    HTHWorld world;
    HTHAABB valid = {{-2.0F, -1.0F, -3.0F}, {4.0F, 5.0F, 6.0F}};
    HTHAABB invalid = {{0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 1.0F}};
    HTHAABB reversed = {{2.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}};
    HTHAABB not_finite = {{0.0F, 0.0F, 0.0F}, {INFINITY, 1.0F, 1.0F}};
    HTHAABB not_a_number = {{0.0F, NAN, 0.0F}, {1.0F, 1.0F, 1.0F}};
    HTHWorldSpawn spawn = {{1.0F, 2.0F, 3.0F}, 0.5F};
    HTHWorldSpawn invalid_spawn = {{NAN, 0.0F, 0.0F}, 0.0F};

    assert(hth_world_init(&world));
    assert(!hth_world_add_static_object(
        &world, invalid, HTH_WORLD_OBJECT_COLLIDABLE,
        HTH_WORLD_VISUAL_WALL));
    assert(!hth_world_add_static_object(
        &world, reversed, HTH_WORLD_OBJECT_COLLIDABLE,
        HTH_WORLD_VISUAL_WALL));
    assert(!hth_world_add_static_object(
        &world, not_finite, HTH_WORLD_OBJECT_COLLIDABLE,
        HTH_WORLD_VISUAL_WALL));
    assert(!hth_world_add_static_object(
        &world, not_a_number, HTH_WORLD_OBJECT_COLLIDABLE,
        HTH_WORLD_VISUAL_WALL));
    assert(!hth_world_add_static_object(
        &world, valid, 1U << 12, HTH_WORLD_VISUAL_WALL));
    assert(!hth_world_add_static_object(
        &world, valid, HTH_WORLD_OBJECT_VISIBLE,
        HTH_WORLD_VISUAL_COUNT));
    assert(!hth_world_set_default_spawn(&world, invalid_spawn));
    assert(hth_world_set_default_spawn(&world, spawn));
    assert(!hth_world_set_default_spawn(&world, spawn));
    assert(hth_world_add_static_object(
        &world, valid,
        HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
        HTH_WORLD_VISUAL_BOX));
    assert(world.object_count == 1U);
    assert(hth_world_finalize(&world));
    assert(!hth_world_add_static_object(
        &world, valid, HTH_WORLD_OBJECT_VISIBLE, HTH_WORLD_VISUAL_BOX));
    assert(!hth_world_set_default_spawn(&world, spawn));
    hth_world_shutdown(&world);
}

static void test_bounds_and_independent_flags(void)
{
    HTHWorld world;
    HTHAABB bounds;
    const HTHWorldStaticObject *object;

    assert(hth_world_init(&world));
    assert(hth_world_add_static_object(
        &world, (HTHAABB){{-2.0F, 1.0F, -3.0F}, {0.0F, 2.0F, 4.0F}},
        HTH_WORLD_OBJECT_VISIBLE, HTH_WORLD_VISUAL_WALL));
    assert(hth_world_add_static_object(
        &world, (HTHAABB){{3.0F, -4.0F, 2.0F}, {8.0F, 6.0F, 9.0F}},
        HTH_WORLD_OBJECT_COLLIDABLE, HTH_WORLD_VISUAL_NONE));
    assert(hth_world_add_static_object(
        &world, (HTHAABB){{1.0F, 0.0F, -1.0F}, {2.0F, 1.0F, 0.0F}},
        0U, HTH_WORLD_VISUAL_NONE));
    assert(hth_world_finalize(&world));
    assert(hth_world_static_object_count(&world) == 3U);
    object = hth_world_static_object(&world, 0U);
    assert(object != NULL);
    assert(object->flags == HTH_WORLD_OBJECT_VISIBLE);
    assert(object->visual_class == HTH_WORLD_VISUAL_WALL);
    object = hth_world_static_object(&world, 1U);
    assert(object != NULL);
    assert(object->flags == HTH_WORLD_OBJECT_COLLIDABLE);
    assert(object->visual_class == HTH_WORLD_VISUAL_NONE);
    assert(hth_world_bounds(&world, &bounds));
    assert_vec3(bounds.min, hth_vec3(-2.0F, -4.0F, -3.0F));
    assert_vec3(bounds.max, hth_vec3(8.0F, 6.0F, 9.0F));
    hth_world_shutdown(&world);
}

static void test_collision_extraction_and_trace(void)
{
    HTHWorld world;
    HTHCollisionWorld collision;
    HTHTrace trace;
    HTHVec3 extents_min = {-0.1F, -0.1F, -0.1F};
    HTHVec3 extents_max = {0.1F, 0.1F, 0.1F};

    assert(hth_world_init(&world));
    assert(hth_world_add_static_object(
        &world, (HTHAABB){{0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}},
        HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
        HTH_WORLD_VISUAL_BOX));
    assert(hth_world_add_static_object(
        &world, (HTHAABB){{3.0F, 0.0F, 0.0F}, {4.0F, 1.0F, 1.0F}},
        HTH_WORLD_OBJECT_VISIBLE, HTH_WORLD_VISUAL_WALL));
    assert(hth_world_add_static_object(
        &world, (HTHAABB){{6.0F, 0.0F, 0.0F}, {7.0F, 1.0F, 1.0F}},
        HTH_WORLD_OBJECT_COLLIDABLE, HTH_WORLD_VISUAL_NONE));
    assert(hth_world_finalize(&world));
    assert(hth_collision_world_build_from_world(&collision, &world));
    assert(collision.obstacle_count == 2U);
    assert_vec3(collision.obstacles[0].min, hth_vec3(0.0F, 0.0F, 0.0F));
    assert_vec3(collision.obstacles[1].min, hth_vec3(6.0F, 0.0F, 0.0F));
    hth_world_shutdown(&world);
    assert(hth_collision_world_trace_aabb(
        &collision, hth_vec3(2.0F, 0.5F, 0.5F),
        hth_vec3(5.0F, 0.5F, 0.5F), extents_min, extents_max, &trace));
    assert(close_enough(trace.fraction, 1.0F));
    assert(hth_collision_world_trace_aabb(
        &collision, hth_vec3(-1.0F, 0.5F, 0.5F),
        hth_vec3(2.0F, 0.5F, 0.5F), extents_min, extents_max, &trace));
    assert(trace.fraction < 1.0F);
    assert(trace.obstacle_index == 0U);
    assert(hth_collision_world_trace_aabb(
        &collision, hth_vec3(5.0F, 0.5F, 0.5F),
        hth_vec3(8.0F, 0.5F, 0.5F), extents_min, extents_max, &trace));
    assert(trace.fraction < 1.0F);
    assert(trace.obstacle_index == 1U);
}

int main(void)
{
    test_empty_world_lifecycle();
    test_validation_and_immutability();
    test_bounds_and_independent_flags();
    test_collision_extraction_and_trace();
    return 0;
}
