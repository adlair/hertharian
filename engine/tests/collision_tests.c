#include "collision_world.h"

#include <assert.h>
#include <math.h>

static bool close_enough(float left, float right)
{
    return fabsf(left - right) <= 1.0e-4F;
}

static HTHCollisionWorld one_box(HTHAABB bounds)
{
    HTHCollisionWorld world = {0};
    world.obstacles[0] = bounds;
    world.obstacle_count = 1;
    assert(hth_collision_world_is_valid(&world));
    return world;
}

static void test_aabb_touching(void)
{
    HTHAABB left = {{0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}};
    HTHAABB touching = {{1.0F, 0.0F, 0.0F}, {2.0F, 1.0F, 1.0F}};
    HTHAABB penetrating = {{0.9F, 0.0F, 0.0F}, {2.0F, 1.0F, 1.0F}};
    assert(!hth_aabb_intersects(&left, &touching));
    assert(hth_aabb_intersects(&left, &penetrating));
}

static void test_floor_landing(void)
{
    HTHCollisionWorld world = one_box(
        (HTHAABB){{-10.0F, -1.0F, -10.0F}, {10.0F, 0.0F, 10.0F}});
    HTHPlayerBody body;

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.2F, 0.0F)));
    body.velocity.y = -3.0F;
    assert(hth_collision_world_move_body(
        &world, &body, hth_vec3(0.0F, -0.3F, 0.0F)));
    assert(close_enough(body.position.y, 0.0F));
    assert(close_enough(body.velocity.y, 0.0F));
    assert(body.grounded);
    assert(!hth_collision_world_body_penetrates(&world, &body));
}

static void test_wall_and_slide(void)
{
    HTHCollisionWorld world = one_box(
        (HTHAABB){{1.0F, 0.0F, -5.0F}, {2.0F, 3.0F, 5.0F}});
    HTHPlayerBody body;

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.velocity = hth_vec3(4.0F, 0.0F, -2.0F);
    assert(hth_collision_world_move_body(
        &world, &body, hth_vec3(1.0F, 0.0F, -0.5F)));
    assert(close_enough(body.position.x, 0.7F));
    assert(close_enough(body.position.z, -0.5F));
    assert(close_enough(body.velocity.x, 0.0F));
    assert(close_enough(body.velocity.z, -2.0F));
    assert(!hth_collision_world_body_penetrates(&world, &body));
}

static void test_isolated_box(void)
{
    HTHCollisionWorld world = one_box(
        (HTHAABB){{-1.0F, 0.0F, -2.0F}, {1.0F, 1.5F, -1.0F}});
    HTHPlayerBody body;

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.velocity.z = -4.0F;
    assert(hth_collision_world_move_body(
        &world, &body, hth_vec3(0.0F, 0.0F, -1.5F)));
    assert(close_enough(body.position.z, -0.7F));
    assert(close_enough(body.velocity.z, 0.0F));
    assert(!hth_collision_world_body_penetrates(&world, &body));
}

static void test_lateral_overlap_is_not_landing(void)
{
    HTHCollisionWorld world = {0};
    HTHPlayerBody body;

    /* The later X correction leaves overlap with an earlier obstacle. */
    world.obstacles[0] = (HTHAABB){
        {1.5F, 0.0F, 0.0F}, {1.7F, 1.0F, 2.0F}
    };
    world.obstacles[1] = (HTHAABB){
        {2.0F, 0.0F, 0.0F}, {3.0F, 2.0F, 2.0F}
    };
    world.obstacle_count = 2;
    assert(hth_collision_world_is_valid(&world));
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 1.0F)));
    body.velocity = hth_vec3(2.1F, -1.0F, 0.1F);

    assert(hth_collision_world_move_body(
        &world, &body, hth_vec3(2.1F, -0.01F, 0.1F)));
    assert(close_enough(body.position.y, -0.01F));
    assert(!body.grounded);
    assert(!hth_collision_world_body_penetrates(&world, &body));
}

int main(void)
{
    test_aabb_touching();
    test_floor_landing();
    test_wall_and_slide();
    test_isolated_box();
    test_lateral_overlap_is_not_landing();
    return 0;
}
