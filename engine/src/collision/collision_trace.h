#ifndef HTH_COLLISION_TRACE_H
#define HTH_COLLISION_TRACE_H

#include "collision_world.h"
#include "hth_math.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    bool hit;
    bool start_solid;
    bool all_solid;
    float fraction;
    HTHVec3 end_position;
    HTHVec3 normal;
    size_t obstacle_index;
} HTHTrace;

bool hth_collision_world_trace_aabb(const HTHCollisionWorld *world,
                                    HTHVec3 start,
                                    HTHVec3 end,
                                    HTHVec3 mins,
                                    HTHVec3 maxs,
                                    HTHTrace *out_trace);

bool hth_collision_world_trace_segment(const HTHCollisionWorld *world,
                                       HTHVec3 start,
                                       HTHVec3 end,
                                       HTHTrace *out_trace);

#endif
