#include "collision_world.h"

#include <assert.h>

static void test_aabb_touching(void)
{
    HTHAABB left = {{0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}};
    HTHAABB touching = {{1.0F, 0.0F, 0.0F}, {2.0F, 1.0F, 1.0F}};
    HTHAABB penetrating = {{0.9F, 0.0F, 0.0F}, {2.0F, 1.0F, 1.0F}};

    assert(hth_aabb_is_valid(&left));
    assert(!hth_aabb_intersects(&left, &touching));
    assert(hth_aabb_intersects(&left, &penetrating));
}

static void test_bootstrap_world(void)
{
    HTHCollisionWorld world;
    size_t index;

    assert(hth_collision_world_init_bootstrap(&world));
    assert(hth_collision_world_is_valid(&world));
    assert(world.obstacle_count > 1);
    for (index = 0; index < world.obstacle_count; ++index) {
        assert(hth_aabb_is_valid(&world.obstacles[index]));
    }
    world.obstacles[0].max.x = world.obstacles[0].min.x;
    assert(!hth_collision_world_is_valid(&world));
}

int main(void)
{
    test_aabb_touching();
    test_bootstrap_world();
    return 0;
}
