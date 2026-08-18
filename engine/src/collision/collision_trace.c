#include "collision_trace.h"

#include <float.h>
#include <math.h>
#include <stdint.h>

typedef struct {
    bool hit;
    bool start_solid;
    float fraction;
    HTHVec3 normal;
} HTHObstacleTrace;

typedef struct {
    bool hit;
    bool start_solid;
    double fraction;
    HTHVec3 normal;
} SegmentObstacleTrace;

static bool finite_vec3(HTHVec3 vector)
{
    return isfinite(vector.x) && isfinite(vector.y) && isfinite(vector.z);
}

static bool valid_extents(HTHVec3 mins, HTHVec3 maxs)
{
    return finite_vec3(mins) && finite_vec3(maxs) &&
           mins.x < maxs.x && mins.y < maxs.y && mins.z < maxs.z;
}

static bool point_strictly_inside(HTHVec3 point, HTHVec3 minimum,
                                  HTHVec3 maximum)
{
    return point.x > minimum.x && point.x < maximum.x &&
           point.y > minimum.y && point.y < maximum.y &&
           point.z > minimum.z && point.z < maximum.z;
}

static bool update_slab(float start, float delta, float minimum,
                        float maximum, HTHVec3 negative_normal,
                        HTHVec3 positive_normal, float *entry,
                        float *exit, HTHVec3 *normal)
{
    float axis_entry;
    float axis_exit;
    HTHVec3 axis_normal;

    if (delta == 0.0F) {
        /* A stationary coordinate on a slab boundary is only touching. */
        return start > minimum && start < maximum;
    }
    if (delta > 0.0F) {
        axis_entry = (minimum - start) / delta;
        axis_exit = (maximum - start) / delta;
        axis_normal = negative_normal;
    } else {
        axis_entry = (maximum - start) / delta;
        axis_exit = (minimum - start) / delta;
        axis_normal = positive_normal;
    }
    if (axis_entry > *entry) {
        *entry = axis_entry;
        *normal = axis_normal;
    }
    if (axis_exit < *exit) {
        *exit = axis_exit;
    }
    return *entry <= *exit;
}

static HTHObstacleTrace trace_obstacle(HTHVec3 start, HTHVec3 end,
                                       HTHVec3 mins, HTHVec3 maxs,
                                       const HTHAABB *obstacle)
{
    HTHObstacleTrace result = {0};
    HTHVec3 delta = hth_vec3_subtract(end, start);
    HTHVec3 expanded_min = hth_vec3_subtract(obstacle->min, maxs);
    HTHVec3 expanded_max = hth_vec3_subtract(obstacle->max, mins);
    float entry = -FLT_MAX;
    float exit = FLT_MAX;

    result.fraction = 1.0F;
    result.start_solid = point_strictly_inside(
        start, expanded_min, expanded_max);
    if (result.start_solid) {
        result.hit = true;
        result.fraction = 0.0F;
        return result;
    }
    if (delta.x == 0.0F && delta.y == 0.0F && delta.z == 0.0F) {
        return result;
    }
    if (!update_slab(start.x, delta.x, expanded_min.x, expanded_max.x,
                     hth_vec3(-1.0F, 0.0F, 0.0F),
                     hth_vec3(1.0F, 0.0F, 0.0F),
                     &entry, &exit, &result.normal) ||
        !update_slab(start.y, delta.y, expanded_min.y, expanded_max.y,
                     hth_vec3(0.0F, -1.0F, 0.0F),
                     hth_vec3(0.0F, 1.0F, 0.0F),
                     &entry, &exit, &result.normal) ||
        !update_slab(start.z, delta.z, expanded_min.z, expanded_max.z,
                     hth_vec3(0.0F, 0.0F, -1.0F),
                     hth_vec3(0.0F, 0.0F, 1.0F),
                     &entry, &exit, &result.normal)) {
        return result;
    }
    if (entry >= 0.0F && entry <= 1.0F && exit >= entry) {
        result.hit = true;
        result.fraction = entry;
    }
    return result;
}

static bool point_inside_any_obstacle(const HTHCollisionWorld *world,
                                      HTHVec3 point, HTHVec3 mins,
                                      HTHVec3 maxs)
{
    size_t index;

    for (index = 0; index < world->obstacle_count; ++index) {
        HTHVec3 expanded_min = hth_vec3_subtract(
            world->obstacles[index].min, maxs);
        HTHVec3 expanded_max = hth_vec3_subtract(
            world->obstacles[index].max, mins);
        if (point_strictly_inside(point, expanded_min, expanded_max)) {
            return true;
        }
    }
    return false;
}

bool hth_collision_world_trace_aabb(const HTHCollisionWorld *world,
                                    HTHVec3 start,
                                    HTHVec3 end,
                                    HTHVec3 mins,
                                    HTHVec3 maxs,
                                    HTHTrace *out_trace)
{
    HTHTrace result = {0};
    HTHVec3 delta;
    size_t index;

    if (!hth_collision_world_is_valid(world) || !finite_vec3(start) ||
        !finite_vec3(end) || !valid_extents(mins, maxs) ||
        out_trace == NULL) {
        return false;
    }
    result.fraction = 1.0F;
    result.end_position = end;
    result.obstacle_index = SIZE_MAX;
    for (index = 0; index < world->obstacle_count; ++index) {
        HTHObstacleTrace obstacle_trace = trace_obstacle(
            start, end, mins, maxs, &world->obstacles[index]);

        if (obstacle_trace.start_solid) {
            if (!result.start_solid) {
                result.obstacle_index = index;
            }
            result.hit = true;
            result.start_solid = true;
            result.fraction = 0.0F;
            result.end_position = start;
            result.normal = hth_vec3(0.0F, 0.0F, 0.0F);
        } else if (!result.start_solid && obstacle_trace.hit &&
                   obstacle_trace.fraction < result.fraction) {
            result.hit = true;
            result.fraction = obstacle_trace.fraction;
            result.normal = obstacle_trace.normal;
            result.obstacle_index = index;
        }
    }
    if (result.start_solid) {
        result.all_solid = point_inside_any_obstacle(
            world, end, mins, maxs);
    } else if (result.hit) {
        delta = hth_vec3_subtract(end, start);
        result.end_position = hth_vec3_add(
            start, hth_vec3_scale(delta, result.fraction));
    }
    *out_trace = result;
    return true;
}

static bool segment_world_is_valid(const HTHCollisionWorld *world)
{
    size_t index;

    if (world == NULL ||
        world->obstacle_count > HTH_COLLISION_WORLD_MAX_OBSTACLES) {
        return false;
    }
    for (index = 0U; index < world->obstacle_count; ++index) {
        if (!hth_aabb_is_valid(&world->obstacles[index])) {
            return false;
        }
    }
    return true;
}

static bool point_strictly_inside_double(HTHVec3 point,
                                         const HTHAABB *obstacle)
{
    return (double)point.x > (double)obstacle->min.x &&
           (double)point.x < (double)obstacle->max.x &&
           (double)point.y > (double)obstacle->min.y &&
           (double)point.y < (double)obstacle->max.y &&
           (double)point.z > (double)obstacle->min.z &&
           (double)point.z < (double)obstacle->max.z;
}

static bool update_segment_slab(double start, double delta, double minimum,
                                double maximum, HTHVec3 negative_normal,
                                HTHVec3 positive_normal, double *entry,
                                double *exit, HTHVec3 *normal)
{
    double axis_entry;
    double axis_exit;
    HTHVec3 axis_normal;

    if (delta == 0.0) {
        return start > minimum && start < maximum;
    }
    if (delta > 0.0) {
        axis_entry = (minimum - start) / delta;
        axis_exit = (maximum - start) / delta;
        axis_normal = negative_normal;
    } else {
        axis_entry = (maximum - start) / delta;
        axis_exit = (minimum - start) / delta;
        axis_normal = positive_normal;
    }
    if (axis_entry > *entry) {
        *entry = axis_entry;
        *normal = axis_normal;
    }
    if (axis_exit < *exit) {
        *exit = axis_exit;
    }
    return *entry <= *exit;
}

static SegmentObstacleTrace trace_segment_obstacle(
    HTHVec3 start, HTHVec3 end, const HTHAABB *obstacle)
{
    SegmentObstacleTrace result = {0};
    const double start_x = (double)start.x;
    const double start_y = (double)start.y;
    const double start_z = (double)start.z;
    const double delta_x = (double)end.x - start_x;
    const double delta_y = (double)end.y - start_y;
    const double delta_z = (double)end.z - start_z;
    double entry = -DBL_MAX;
    double exit = DBL_MAX;

    result.fraction = 1.0;
    result.start_solid = point_strictly_inside_double(start, obstacle);
    if (result.start_solid) {
        result.hit = true;
        result.fraction = 0.0;
        return result;
    }
    if (delta_x == 0.0 && delta_y == 0.0 && delta_z == 0.0) {
        return result;
    }
    if (!update_segment_slab(
            start_x, delta_x, (double)obstacle->min.x,
            (double)obstacle->max.x, hth_vec3(-1.0F, 0.0F, 0.0F),
            hth_vec3(1.0F, 0.0F, 0.0F), &entry, &exit,
            &result.normal) ||
        !update_segment_slab(
            start_y, delta_y, (double)obstacle->min.y,
            (double)obstacle->max.y, hth_vec3(0.0F, -1.0F, 0.0F),
            hth_vec3(0.0F, 1.0F, 0.0F), &entry, &exit,
            &result.normal) ||
        !update_segment_slab(
            start_z, delta_z, (double)obstacle->min.z,
            (double)obstacle->max.z, hth_vec3(0.0F, 0.0F, -1.0F),
            hth_vec3(0.0F, 0.0F, 1.0F), &entry, &exit,
            &result.normal)) {
        return result;
    }
    if (entry >= 0.0 && entry <= 1.0 && exit >= entry) {
        result.hit = true;
        result.fraction = entry;
    }
    return result;
}

static bool point_inside_any_segment_obstacle(
    const HTHCollisionWorld *world, HTHVec3 point)
{
    size_t index;

    for (index = 0U; index < world->obstacle_count; ++index) {
        if (point_strictly_inside_double(point, &world->obstacles[index])) {
            return true;
        }
    }
    return false;
}

static HTHVec3 segment_position(HTHVec3 start, HTHVec3 end,
                                double fraction)
{
    const double x = (double)start.x +
                     ((double)end.x - (double)start.x) * fraction;
    const double y = (double)start.y +
                     ((double)end.y - (double)start.y) * fraction;
    const double z = (double)start.z +
                     ((double)end.z - (double)start.z) * fraction;

    return hth_vec3((float)x, (float)y, (float)z);
}

bool hth_collision_world_trace_segment(const HTHCollisionWorld *world,
                                       HTHVec3 start,
                                       HTHVec3 end,
                                       HTHTrace *out_trace)
{
    HTHTrace result = {0};
    double result_fraction = 1.0;
    size_t index;

    if (!segment_world_is_valid(world) || !finite_vec3(start) ||
        !finite_vec3(end) || out_trace == NULL) {
        return false;
    }
    result.fraction = 1.0F;
    result.end_position = end;
    result.obstacle_index = SIZE_MAX;
    for (index = 0U; index < world->obstacle_count; ++index) {
        SegmentObstacleTrace obstacle_trace = trace_segment_obstacle(
            start, end, &world->obstacles[index]);

        if (obstacle_trace.start_solid) {
            if (!result.start_solid) {
                result.obstacle_index = index;
            }
            result.hit = true;
            result.start_solid = true;
            result.fraction = 0.0F;
            result_fraction = 0.0;
            result.end_position = start;
            result.normal = hth_vec3(0.0F, 0.0F, 0.0F);
        } else if (!result.start_solid && obstacle_trace.hit &&
                   (!result.hit ||
                    obstacle_trace.fraction < result_fraction)) {
            result.hit = true;
            result_fraction = obstacle_trace.fraction;
            result.fraction = (float)obstacle_trace.fraction;
            result.normal = obstacle_trace.normal;
            result.obstacle_index = index;
        }
    }
    if (result.start_solid) {
        result.all_solid = point_inside_any_segment_obstacle(world, end);
    } else if (result.hit) {
        result.end_position = segment_position(start, end, result_fraction);
    }
    *out_trace = result;
    return true;
}
