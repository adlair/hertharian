#include "bootstrap_world.h"

typedef struct {
    HTHAABB bounds;
    HTHWorldVisualClass visual_class;
} HTHBootstrapObject;

bool hth_bootstrap_world_create(HTHWorld *world)
{
    const HTHBootstrapObject bootstrap_objects[] = {
        {{{-20.0F, -1.0F, -20.0F}, {20.0F, 0.0F, 20.0F}},
         HTH_WORLD_VISUAL_FLOOR},
        {{{-6.0F, 0.0F, -14.0F}, {-5.0F, 3.0F, 2.0F}},
         HTH_WORLD_VISUAL_WALL},
        {{{5.0F, 0.0F, -14.0F}, {6.0F, 3.0F, 2.0F}},
         HTH_WORLD_VISUAL_WALL},
        {{{-6.0F, 0.0F, -14.0F}, {1.0F, 3.0F, -13.0F}},
         HTH_WORLD_VISUAL_CORNER},
        {{{-1.0F, 0.0F, -2.5F}, {1.0F, 0.20F, -1.0F}},
         HTH_WORLD_VISUAL_LOW_STEP},
        {{{2.0F, 0.0F, -3.5F}, {4.0F, 0.60F, -2.0F}},
         HTH_WORLD_VISUAL_HIGH_LEDGE},
        {{{-4.0F, 0.0F, -5.5F}, {-2.5F, 1.20F, -4.0F}},
         HTH_WORLD_VISUAL_BOX},
        {{{-1.0F, 0.0F, -7.0F}, {1.0F, 0.30F, -5.0F}},
         HTH_WORLD_VISUAL_LIMIT_STEP},
        {{{2.5F, 0.0F, -10.0F}, {3.5F, 2.5F, -5.0F}},
         HTH_WORLD_VISUAL_PLATFORM},
        {{{0.5F, 0.0F, -10.0F}, {1.5F, 2.5F, -9.0F}},
         HTH_WORLD_VISUAL_CORRIDOR_CORNER},
    };
    HTHWorldSpawn spawn = {{0.0F, 0.05F, 3.0F}, 0.0F};
    size_t index;

    if (!hth_world_init(world)) {
        return false;
    }
    for (index = 0;
         index < sizeof(bootstrap_objects) / sizeof(bootstrap_objects[0]);
         ++index) {
        if (!hth_world_add_static_object(
                world, bootstrap_objects[index].bounds,
                HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
                bootstrap_objects[index].visual_class)) {
            hth_world_shutdown(world);
            return false;
        }
    }
    if (!hth_world_set_default_spawn(world, spawn) ||
        !hth_world_finalize(world)) {
        hth_world_shutdown(world);
        return false;
    }
    return true;
}
