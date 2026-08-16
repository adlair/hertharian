#include "dynamic_collision.h"

#include "collision_trace.h"

#include <math.h>

#define HTH_DYNAMIC_COLLISION_MAX_BUMPS 4U

static bool vector_is_finite(HTHVec3 vector)
{
    return isfinite(vector.x) && isfinite(vector.y) && isfinite(vector.z);
}

static bool vector_is_zero(HTHVec3 vector)
{
    return vector.x == 0.0F && vector.y == 0.0F && vector.z == 0.0F;
}

static bool vector_equal(HTHVec3 left, HTHVec3 right)
{
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

static HTHVec3 clip_velocity(HTHVec3 velocity, HTHVec3 normal)
{
    float entering_speed = hth_vec3_dot(velocity, normal);

    if (entering_speed >= 0.0F) {
        return velocity;
    }
    return hth_vec3_subtract(
        velocity, hth_vec3_scale(normal, entering_speed));
}

bool hth_dynamic_collision_move(HTHDynamicBodyStore *bodies,
                                const HTHEntityRegistry *entities,
                                HTHSpatialStore *spatial,
                                const HTHCollisionWorld *collision_world,
                                HTHEntityHandle entity,
                                float dt,
                                HTHDynamicCollisionResult *out_result)
{
    HTHDynamicCollisionResult result = {false, false, false};
    HTHDynamicBody body;
    HTHSpatialTransform transform;
    HTHVec3 original_position;
    HTHVec3 original_velocity;
    HTHVec3 position;
    HTHVec3 velocity;
    HTHVec3 mins;
    HTHVec3 maxs;
    float remaining_time;
    unsigned int bump;

    if (out_result == NULL) {
        return false;
    }
    *out_result = result;
    if (bodies == NULL || entities == NULL || spatial == NULL ||
        !hth_collision_world_is_valid(collision_world) || !isfinite(dt) ||
        dt < 0.0F ||
        !hth_dynamic_body_get(bodies, entities, entity, &body) ||
        !hth_spatial_store_get(spatial, entities, entity, &transform) ||
        !vector_is_finite(body.half_extents) ||
        body.half_extents.x <= 0.0F || body.half_extents.y <= 0.0F ||
        body.half_extents.z <= 0.0F ||
        !vector_is_finite(body.velocity)) {
        return false;
    }
    if (dt == 0.0F) {
        return true;
    }

    original_position = transform.position;
    original_velocity = body.velocity;
    position = original_position;
    velocity = original_velocity;
    mins = hth_vec3(-body.half_extents.x, -body.half_extents.y,
                    -body.half_extents.z);
    maxs = body.half_extents;
    remaining_time = dt;

    for (bump = 0U;
         bump < HTH_DYNAMIC_COLLISION_MAX_BUMPS && remaining_time > 0.0F;
         ++bump) {
        HTHTrace trace;
        HTHVec3 displacement = hth_vec3_scale(velocity, remaining_time);
        HTHVec3 end = hth_vec3_add(position, displacement);

        if (!vector_is_finite(displacement) || !vector_is_finite(end) ||
            !hth_collision_world_trace_aabb(
                collision_world, position, end, mins, maxs, &trace)) {
            return false;
        }
        if (trace.start_solid) {
            result.collided = true;
            result.start_solid = true;
            *out_result = result;
            return true;
        }
        position = trace.end_position;
        if (!trace.hit) {
            remaining_time = 0.0F;
            break;
        }
        result.collided = true;
        remaining_time *= 1.0F - trace.fraction;
        velocity = clip_velocity(velocity, trace.normal);
        if (vector_is_zero(velocity)) {
            remaining_time = 0.0F;
            break;
        }
    }
    if (remaining_time > 0.0F) {
        velocity = hth_vec3(0.0F, 0.0F, 0.0F);
    }
    if (!vector_is_finite(position) || !vector_is_finite(velocity)) {
        return false;
    }

    result.moved = !vector_equal(position, original_position);
    if (!vector_equal(velocity, original_velocity) &&
        !hth_dynamic_body_set_velocity(
            bodies, entities, entity, velocity)) {
        return false;
    }
    if (result.moved) {
        transform.position = position;
        if (!hth_spatial_store_set(spatial, entities, entity, &transform)) {
            if (!vector_equal(velocity, original_velocity)) {
                (void)hth_dynamic_body_set_velocity(
                    bodies, entities, entity, original_velocity);
            }
            return false;
        }
    }
    *out_result = result;
    return true;
}
