#include "collision_world.h"

#include <stddef.h>
#include <string.h>

bool hth_collision_world_build_from_world(HTHCollisionWorld *collision,
                                          const HTHWorld *world)
{
    size_t index;

    if (collision == NULL || !hth_world_is_finalized(world)) {
        return false;
    }
    memset(collision, 0, sizeof(*collision));
    for (index = 0; index < hth_world_static_object_count(world); ++index) {
        const HTHWorldStaticObject *object =
            hth_world_static_object(world, index);

        if (object == NULL) {
            return false;
        }
        if ((object->flags & HTH_WORLD_OBJECT_COLLIDABLE) == 0U ||
            object->collision_shape != HTH_WORLD_COLLISION_AABB) {
            continue;
        }
        if (collision->obstacle_count ==
            HTH_COLLISION_WORLD_MAX_OBSTACLES) {
            memset(collision, 0, sizeof(*collision));
            return false;
        }
        collision->obstacles[collision->obstacle_count++] = object->bounds;
    }
    return hth_collision_world_is_valid(collision);
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
