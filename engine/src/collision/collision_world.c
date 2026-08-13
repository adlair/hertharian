#include "collision_world.h"

#include <stddef.h>
#include <string.h>

static const HTHAABB bootstrap_obstacles[] = {
    {{-20.0F, -1.0F, -20.0F}, {20.0F, 0.0F, 20.0F}},
    {{-6.0F, 0.0F, -14.0F}, {-5.0F, 3.0F, 2.0F}},
    {{5.0F, 0.0F, -14.0F}, {6.0F, 3.0F, 2.0F}},
    {{-6.0F, 0.0F, -14.0F}, {1.0F, 3.0F, -13.0F}},
    {{-1.0F, 0.0F, -2.5F}, {1.0F, 0.20F, -1.0F}},
    {{2.0F, 0.0F, -3.5F}, {4.0F, 0.60F, -2.0F}},
    {{-4.0F, 0.0F, -5.5F}, {-2.5F, 1.20F, -4.0F}},
    {{-1.0F, 0.0F, -7.0F}, {1.0F, 0.30F, -5.0F}},
    {{2.5F, 0.0F, -10.0F}, {3.5F, 2.5F, -5.0F}},
    {{0.5F, 0.0F, -10.0F}, {1.5F, 2.5F, -9.0F}},
};

bool hth_collision_world_init_bootstrap(HTHCollisionWorld *world)
{
    size_t index;

    if (world == NULL ||
        sizeof(bootstrap_obstacles) / sizeof(bootstrap_obstacles[0]) >
            HTH_COLLISION_WORLD_MAX_OBSTACLES) {
        return false;
    }
    memset(world, 0, sizeof(*world));
    world->obstacle_count =
        sizeof(bootstrap_obstacles) / sizeof(bootstrap_obstacles[0]);
    for (index = 0; index < world->obstacle_count; ++index) {
        world->obstacles[index] = bootstrap_obstacles[index];
    }
    return hth_collision_world_is_valid(world);
}

bool hth_collision_world_is_valid(const HTHCollisionWorld *world)
{
    size_t index;

    if (world == NULL || world->obstacle_count == 0 ||
        world->obstacle_count > HTH_COLLISION_WORLD_MAX_OBSTACLES) {
        return false;
    }
    for (index = 0; index < world->obstacle_count; ++index) {
        if (!hth_aabb_is_valid(&world->obstacles[index])) {
            return false;
        }
    }
    return true;
}
