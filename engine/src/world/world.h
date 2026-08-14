#ifndef HTH_WORLD_H
#define HTH_WORLD_H

#include "aabb.h"
#include "hth_math.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    HTH_WORLD_VISUAL_NONE = 0,
    HTH_WORLD_VISUAL_FLOOR,
    HTH_WORLD_VISUAL_WALL,
    HTH_WORLD_VISUAL_BOX,
    HTH_WORLD_VISUAL_LOW_STEP,
    HTH_WORLD_VISUAL_LIMIT_STEP,
    HTH_WORLD_VISUAL_HIGH_LEDGE,
    HTH_WORLD_VISUAL_PLATFORM,
    HTH_WORLD_VISUAL_CORNER,
    HTH_WORLD_VISUAL_CORRIDOR_CORNER,
    HTH_WORLD_VISUAL_COUNT
} HTHWorldVisualClass;

typedef enum {
    HTH_WORLD_OBJECT_COLLIDABLE = 1U << 0,
    HTH_WORLD_OBJECT_VISIBLE = 1U << 1
} HTHWorldObjectFlags;

typedef struct {
    HTHAABB bounds;
    uint32_t flags;
    HTHWorldVisualClass visual_class;
} HTHWorldStaticObject;

typedef struct {
    HTHVec3 position;
    float yaw_radians;
} HTHWorldSpawn;

typedef struct HTHWorld {
    HTHWorldStaticObject *objects;
    size_t object_count;
    size_t object_capacity;
    HTHAABB bounds;
    HTHWorldSpawn default_spawn;
    bool has_bounds;
    bool has_default_spawn;
    bool finalized;
} HTHWorld;

bool hth_world_init(HTHWorld *world);
void hth_world_shutdown(HTHWorld *world);
bool hth_world_add_static_object(HTHWorld *world, HTHAABB bounds,
                                 uint32_t flags,
                                 HTHWorldVisualClass visual_class);
bool hth_world_set_default_spawn(HTHWorld *world, HTHWorldSpawn spawn);
bool hth_world_finalize(HTHWorld *world);
bool hth_world_is_finalized(const HTHWorld *world);
size_t hth_world_static_object_count(const HTHWorld *world);
const HTHWorldStaticObject *hth_world_static_object(const HTHWorld *world,
                                                    size_t index);
bool hth_world_bounds(const HTHWorld *world, HTHAABB *out_bounds);
bool hth_world_default_spawn(const HTHWorld *world,
                             HTHWorldSpawn *out_spawn);

#endif
