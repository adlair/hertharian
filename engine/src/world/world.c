#include "world.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t known_flags = HTH_WORLD_OBJECT_COLLIDABLE |
                                    HTH_WORLD_OBJECT_VISIBLE;

static bool valid_visual_class(HTHWorldVisualClass visual_class)
{
    return visual_class >= HTH_WORLD_VISUAL_NONE &&
           visual_class < HTH_WORLD_VISUAL_COUNT;
}

static bool valid_collision_shape(HTHWorldCollisionShape shape)
{
    return shape >= HTH_WORLD_COLLISION_NONE &&
           shape < HTH_WORLD_COLLISION_COUNT;
}

static bool valid_render_shape(HTHWorldRenderShape shape)
{
    return shape >= HTH_WORLD_RENDER_NONE && shape < HTH_WORLD_RENDER_COUNT;
}

static bool valid_static_object(const HTHWorldStaticObject *object)
{
    bool collidable;
    bool visible;

    if (object == NULL || !hth_aabb_is_valid(&object->bounds) ||
        (object->flags & ~known_flags) != 0U ||
        !valid_collision_shape(object->collision_shape) ||
        !valid_render_shape(object->render_shape) ||
        !valid_visual_class(object->visual_class)) {
        return false;
    }
    collidable = (object->flags & HTH_WORLD_OBJECT_COLLIDABLE) != 0U;
    visible = (object->flags & HTH_WORLD_OBJECT_VISIBLE) != 0U;
    return (collidable
                ? object->collision_shape == HTH_WORLD_COLLISION_AABB
                : object->collision_shape == HTH_WORLD_COLLISION_NONE) &&
           (visible
                ? object->render_shape != HTH_WORLD_RENDER_NONE
                : object->render_shape == HTH_WORLD_RENDER_NONE);
}

bool hth_world_init(HTHWorld *world)
{
    if (world == NULL) {
        return false;
    }
    memset(world, 0, sizeof(*world));
    return true;
}

void hth_world_shutdown(HTHWorld *world)
{
    if (world == NULL) {
        return;
    }
    free(world->objects);
    memset(world, 0, sizeof(*world));
}

bool hth_world_add_static_object(HTHWorld *world, HTHAABB bounds,
                                 HTHWorldCollisionShape collision_shape,
                                 HTHWorldRenderShape render_shape,
                                 uint32_t flags,
                                 HTHWorldVisualClass visual_class)
{
    HTHWorldStaticObject object;

    object.bounds = bounds;
    object.collision_shape = collision_shape;
    object.render_shape = render_shape;
    object.flags = flags;
    object.visual_class = visual_class;
    if (world == NULL || world->finalized || !valid_static_object(&object)) {
        return false;
    }
    if (world->object_count == world->object_capacity) {
        size_t new_capacity = world->object_capacity == 0
            ? 8U : world->object_capacity * 2U;
        HTHWorldStaticObject *new_objects;

        if (new_capacity < world->object_capacity ||
            new_capacity > SIZE_MAX / sizeof(*new_objects)) {
            return false;
        }
        new_objects = realloc(world->objects,
                              new_capacity * sizeof(*new_objects));
        if (new_objects == NULL) {
            return false;
        }
        world->objects = new_objects;
        world->object_capacity = new_capacity;
    }
    world->objects[world->object_count++] = object;
    return true;
}

bool hth_world_set_default_spawn(HTHWorld *world, HTHWorldSpawn spawn)
{
    if (world == NULL || world->finalized || world->has_default_spawn ||
        !isfinite(spawn.position.x) || !isfinite(spawn.position.y) ||
        !isfinite(spawn.position.z) || !isfinite(spawn.yaw_radians)) {
        return false;
    }
    world->default_spawn = spawn;
    world->has_default_spawn = true;
    return true;
}

bool hth_world_finalize(HTHWorld *world)
{
    size_t index;

    if (world == NULL || world->finalized) {
        return false;
    }
    world->has_bounds = false;
    for (index = 0; index < world->object_count; ++index) {
        const HTHAABB *bounds;

        if (!valid_static_object(&world->objects[index])) {
            return false;
        }
        bounds = &world->objects[index].bounds;
        if (!world->has_bounds) {
            world->bounds = *bounds;
            world->has_bounds = true;
        } else {
            world->bounds.min.x = fminf(world->bounds.min.x, bounds->min.x);
            world->bounds.min.y = fminf(world->bounds.min.y, bounds->min.y);
            world->bounds.min.z = fminf(world->bounds.min.z, bounds->min.z);
            world->bounds.max.x = fmaxf(world->bounds.max.x, bounds->max.x);
            world->bounds.max.y = fmaxf(world->bounds.max.y, bounds->max.y);
            world->bounds.max.z = fmaxf(world->bounds.max.z, bounds->max.z);
        }
    }
    world->finalized = true;
    return true;
}

bool hth_world_is_finalized(const HTHWorld *world)
{
    return world != NULL && world->finalized;
}

size_t hth_world_static_object_count(const HTHWorld *world)
{
    return hth_world_is_finalized(world) ? world->object_count : 0U;
}

const HTHWorldStaticObject *hth_world_static_object(const HTHWorld *world,
                                                    size_t index)
{
    if (!hth_world_is_finalized(world) || index >= world->object_count) {
        return NULL;
    }
    return &world->objects[index];
}

bool hth_world_bounds(const HTHWorld *world, HTHAABB *out_bounds)
{
    if (!hth_world_is_finalized(world) || !world->has_bounds ||
        out_bounds == NULL) {
        return false;
    }
    *out_bounds = world->bounds;
    return true;
}

bool hth_world_default_spawn(const HTHWorld *world,
                             HTHWorldSpawn *out_spawn)
{
    if (!hth_world_is_finalized(world) || !world->has_default_spawn ||
        out_spawn == NULL) {
        return false;
    }
    *out_spawn = world->default_spawn;
    return true;
}
