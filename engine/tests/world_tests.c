#include "collision_world.h"
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

static HTHAABB bounds_at(float x)
{
    HTHAABB bounds = {{x, 0.0F, 0.0F}, {x + 1.0F, 1.0F, 1.0F}};
    return bounds;
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
    assert(!hth_world_bounds(&world, &bounds));
    assert(!hth_world_finalize(&world));
    hth_world_shutdown(&world);
}

static void test_valid_shape_flag_matrix_and_bounds(void)
{
    HTHWorld world;
    HTHAABB bounds;
    const HTHWorldStaticObject *object;

    assert(hth_world_init(&world));
    assert(hth_world_add_static_object(
        &world, bounds_at(-2.0F), HTH_WORLD_COLLISION_AABB,
        HTH_WORLD_RENDER_BOX,
        HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
        HTH_WORLD_VISUAL_BOX));
    assert(hth_world_add_static_object(
        &world, bounds_at(1.0F), HTH_WORLD_COLLISION_NONE,
        HTH_WORLD_RENDER_WEDGE, HTH_WORLD_OBJECT_VISIBLE,
        HTH_WORLD_VISUAL_WALL));
    assert(hth_world_add_static_object(
        &world, bounds_at(4.0F), HTH_WORLD_COLLISION_AABB,
        HTH_WORLD_RENDER_NONE, HTH_WORLD_OBJECT_COLLIDABLE,
        HTH_WORLD_VISUAL_NONE));
    assert(hth_world_add_static_object(
        &world, bounds_at(7.0F), HTH_WORLD_COLLISION_NONE,
        HTH_WORLD_RENDER_NONE, 0U, HTH_WORLD_VISUAL_NONE));
    assert(hth_world_finalize(&world));
    assert(hth_world_static_object_count(&world) == 4U);
    object = hth_world_static_object(&world, 1U);
    assert(object != NULL);
    assert(object->collision_shape == HTH_WORLD_COLLISION_NONE);
    assert(object->render_shape == HTH_WORLD_RENDER_WEDGE);
    assert(hth_world_bounds(&world, &bounds));
    assert_vec3(bounds.min, hth_vec3(-2.0F, 0.0F, 0.0F));
    assert_vec3(bounds.max, hth_vec3(8.0F, 1.0F, 1.0F));
    hth_world_shutdown(&world);
}

static void test_invalid_objects_and_immutability(void)
{
    HTHWorld world;
    HTHAABB valid = bounds_at(0.0F);
    HTHAABB invalid = {{0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 1.0F}};
    HTHWorldSpawn spawn = {{1.0F, 2.0F, 3.0F}, 0.5F};

    assert(hth_world_init(&world));
    assert(!hth_world_add_static_object(
        &world, invalid, HTH_WORLD_COLLISION_AABB, HTH_WORLD_RENDER_BOX,
        HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
        HTH_WORLD_VISUAL_BOX));
    assert(!hth_world_add_static_object(
        &world, valid, HTH_WORLD_COLLISION_NONE, HTH_WORLD_RENDER_BOX,
        HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
        HTH_WORLD_VISUAL_BOX));
    assert(!hth_world_add_static_object(
        &world, valid, HTH_WORLD_COLLISION_AABB, HTH_WORLD_RENDER_BOX,
        HTH_WORLD_OBJECT_VISIBLE, HTH_WORLD_VISUAL_BOX));
    assert(!hth_world_add_static_object(
        &world, valid, HTH_WORLD_COLLISION_AABB, HTH_WORLD_RENDER_NONE,
        HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
        HTH_WORLD_VISUAL_BOX));
    assert(!hth_world_add_static_object(
        &world, valid, HTH_WORLD_COLLISION_NONE, HTH_WORLD_RENDER_BOX, 0U,
        HTH_WORLD_VISUAL_BOX));
    assert(!hth_world_add_static_object(
        &world, valid, HTH_WORLD_COLLISION_NONE, HTH_WORLD_RENDER_WEDGE, 0U,
        HTH_WORLD_VISUAL_BOX));
    assert(!hth_world_add_static_object(
        &world, valid, HTH_WORLD_COLLISION_COUNT, HTH_WORLD_RENDER_NONE, 0U,
        HTH_WORLD_VISUAL_NONE));
    assert(!hth_world_add_static_object(
        &world, valid, HTH_WORLD_COLLISION_NONE, HTH_WORLD_RENDER_COUNT, 0U,
        HTH_WORLD_VISUAL_NONE));
    assert(!hth_world_add_static_object(
        &world, valid, HTH_WORLD_COLLISION_NONE, HTH_WORLD_RENDER_NONE,
        1U << 12, HTH_WORLD_VISUAL_NONE));
    assert(!hth_world_add_static_object(
        &world, valid, HTH_WORLD_COLLISION_NONE, HTH_WORLD_RENDER_NONE, 0U,
        HTH_WORLD_VISUAL_COUNT));
    assert(hth_world_set_default_spawn(&world, spawn));
    assert(hth_world_add_static_object(
        &world, valid, HTH_WORLD_COLLISION_AABB, HTH_WORLD_RENDER_BOX,
        HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
        HTH_WORLD_VISUAL_BOX));
    world.objects[0].render_shape = HTH_WORLD_RENDER_NONE;
    assert(!hth_world_finalize(&world));
    world.objects[0].render_shape = HTH_WORLD_RENDER_BOX;
    assert(hth_world_finalize(&world));
    assert(!hth_world_add_static_object(
        &world, valid, HTH_WORLD_COLLISION_AABB, HTH_WORLD_RENDER_BOX,
        HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
        HTH_WORLD_VISUAL_BOX));
    assert(!hth_world_set_default_spawn(&world, spawn));
    hth_world_shutdown(&world);
}

static void test_collision_extraction_ignores_render_state(void)
{
    HTHWorld world;
    HTHCollisionWorld collision;

    assert(hth_world_init(&world));
    assert(hth_world_add_static_object(
        &world, bounds_at(0.0F), HTH_WORLD_COLLISION_AABB,
        HTH_WORLD_RENDER_WEDGE,
        HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
        HTH_WORLD_VISUAL_BOX));
    assert(hth_world_add_static_object(
        &world, bounds_at(3.0F), HTH_WORLD_COLLISION_NONE,
        HTH_WORLD_RENDER_WEDGE, HTH_WORLD_OBJECT_VISIBLE,
        HTH_WORLD_VISUAL_BOX));
    assert(hth_world_add_static_object(
        &world, bounds_at(6.0F), HTH_WORLD_COLLISION_AABB,
        HTH_WORLD_RENDER_NONE, HTH_WORLD_OBJECT_COLLIDABLE,
        HTH_WORLD_VISUAL_NONE));
    assert(hth_world_finalize(&world));
    assert(hth_collision_world_build_from_world(&collision, &world));
    assert(collision.obstacle_count == 2U);
    assert_vec3(collision.obstacles[0].min, hth_vec3(0.0F, 0.0F, 0.0F));
    assert_vec3(collision.obstacles[1].min, hth_vec3(6.0F, 0.0F, 0.0F));
    hth_world_shutdown(&world);
}

int main(void)
{
    test_empty_world_lifecycle();
    test_valid_shape_flag_matrix_and_bounds();
    test_invalid_objects_and_immutability();
    test_collision_extraction_ignores_render_state();
    return 0;
}
