#include "collision_world.h"
#include "bootstrap_world.h"

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
    HTHWorld source;
    HTHCollisionWorld collision;
    size_t index;

    assert(hth_bootstrap_world_create(&source));
    assert(hth_collision_world_build_from_world(&collision, &source));
    assert(hth_collision_world_is_valid(&collision));
    assert(collision.obstacle_count > 1);
    for (index = 0; index < collision.obstacle_count; ++index) {
        assert(hth_aabb_is_valid(&collision.obstacles[index]));
    }
    collision.obstacles[0].max.x = collision.obstacles[0].min.x;
    assert(!hth_collision_world_is_valid(&collision));
    hth_world_shutdown(&source);
}

int main(void)
{
    test_aabb_touching();
    test_bootstrap_world();
    return 0;
}
