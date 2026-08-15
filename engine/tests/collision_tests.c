#include "collision_world.h"
#include "world.h"

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

static void test_collision_extraction_from_world(void)
{
    HTHWorld source;
    HTHCollisionWorld collision;
    size_t index;

    assert(hth_world_init(&source));
    assert(hth_world_add_static_object(
        &source, (HTHAABB){{-2.0F, -1.0F, -2.0F}, {2.0F, 0.0F, 2.0F}},
        HTH_WORLD_COLLISION_AABB, HTH_WORLD_RENDER_BOX,
        HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
        HTH_WORLD_VISUAL_FLOOR));
    assert(hth_world_add_static_object(
        &source, (HTHAABB){{1.0F, 0.0F, -1.0F}, {2.0F, 2.0F, 1.0F}},
        HTH_WORLD_COLLISION_AABB, HTH_WORLD_RENDER_NONE,
        HTH_WORLD_OBJECT_COLLIDABLE, HTH_WORLD_VISUAL_NONE));
    assert(hth_world_finalize(&source));
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
    test_collision_extraction_from_world();
    return 0;
}
