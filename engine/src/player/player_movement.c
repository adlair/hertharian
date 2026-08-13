#include "player_movement.h"

#include "collision_trace.h"

#include <math.h>
#include <stddef.h>

#define HTH_MAX_SLIDE_BUMPS 4

typedef struct {
    bool blocked_horizontal;
    bool start_solid;
} HTHSlideResult;

static const float step_height = 0.30F;
static const float ground_probe_distance = 0.04F;
static const float plane_epsilon = 1.0e-4F;
static const float progress_epsilon = 1.0e-6F;
static const float surface_offset = 1.0e-5F;

static void body_extents(const HTHPlayerBody *body, HTHVec3 *mins,
                         HTHVec3 *maxs)
{
    *mins = hth_vec3(-body->half_width, 0.0F, -body->half_width);
    *maxs = hth_vec3(body->half_width, body->height, body->half_width);
}

static HTHVec3 clip_vector(HTHVec3 vector, HTHVec3 normal)
{
    return hth_vec3_subtract(
        vector, hth_vec3_scale(normal, hth_vec3_dot(vector, normal)));
}

static bool duplicate_plane(const HTHVec3 *planes, size_t plane_count,
                            HTHVec3 normal)
{
    size_t index;

    for (index = 0; index < plane_count; ++index) {
        if (hth_vec3_dot(planes[index], normal) > 1.0F - plane_epsilon) {
            return true;
        }
    }
    return false;
}

static HTHVec3 velocity_for_planes(HTHVec3 velocity,
                                   const HTHVec3 *planes,
                                   size_t plane_count)
{
    HTHVec3 result = velocity;
    size_t check;
    size_t first;
    size_t second;

    for (first = 0; first < plane_count; ++first) {
        if (hth_vec3_dot(result, planes[first]) >= 0.0F) {
            continue;
        }
        result = clip_vector(result, planes[first]);
        for (second = 0; second < plane_count; ++second) {
            HTHVec3 crease;

            if (second == first ||
                hth_vec3_dot(result, planes[second]) >= -plane_epsilon) {
                continue;
            }
            if (!hth_vec3_normalize(
                    hth_vec3_cross(planes[first], planes[second]),
                    &crease)) {
                return hth_vec3(0.0F, 0.0F, 0.0F);
            }
            result = hth_vec3_scale(
                crease, hth_vec3_dot(velocity, crease));
            for (check = 0; check < plane_count; ++check) {
                if (hth_vec3_dot(result, planes[check]) <
                    -plane_epsilon) {
                    return hth_vec3(0.0F, 0.0F, 0.0F);
                }
            }
            return result;
        }
    }
    return result;
}

static bool slide_move(HTHPlayerBody *body,
                       const HTHCollisionWorld *world,
                       float duration,
                       HTHSlideResult *out_result)
{
    HTHVec3 maxs;
    HTHVec3 mins;
    HTHVec3 planes[HTH_MAX_SLIDE_BUMPS];
    float remaining_time = duration;
    size_t plane_count = 0;
    unsigned int bump;

    body_extents(body, &mins, &maxs);
    out_result->blocked_horizontal = false;
    out_result->start_solid = false;
    for (bump = 0; bump < HTH_MAX_SLIDE_BUMPS && remaining_time > 0.0F;
         ++bump) {
        HTHTrace trace;
        HTHVec3 end = hth_vec3_add(
            body->position, hth_vec3_scale(body->velocity, remaining_time));

        if (!hth_collision_world_trace_aabb(
                world, body->position, end, mins, maxs, &trace)) {
            return false;
        }
        if (trace.start_solid) {
            body->velocity = hth_vec3(0.0F, 0.0F, 0.0F);
            out_result->start_solid = true;
            return true;
        }
        body->position = trace.end_position;
        if (!trace.hit) {
            return true;
        }
        body->position = hth_vec3_add(
            body->position, hth_vec3_scale(trace.normal, surface_offset));
        if (trace.normal.x != 0.0F || trace.normal.z != 0.0F) {
            out_result->blocked_horizontal = true;
        }
        remaining_time *= 1.0F - trace.fraction;
        if (!duplicate_plane(planes, plane_count, trace.normal)) {
            if (plane_count == HTH_MAX_SLIDE_BUMPS) {
                body->velocity = hth_vec3(0.0F, 0.0F, 0.0F);
                return true;
            }
            planes[plane_count++] = trace.normal;
        }
        body->velocity = velocity_for_planes(
            body->velocity, planes, plane_count);
        if (hth_vec3_dot(body->velocity, body->velocity) <=
            progress_epsilon) {
            body->velocity = hth_vec3(0.0F, 0.0F, 0.0F);
            return true;
        }
    }
    if (remaining_time > 0.0F) {
        body->velocity = hth_vec3(0.0F, 0.0F, 0.0F);
    }
    return true;
}

static bool trace_down_to_ground(HTHPlayerBody *body,
                                 const HTHCollisionWorld *world,
                                 float distance,
                                 bool *out_grounded)
{
    HTHTrace trace;
    HTHVec3 maxs;
    HTHVec3 mins;
    HTHVec3 end = hth_vec3_add(
        body->position, hth_vec3(0.0F, -distance, 0.0F));

    body_extents(body, &mins, &maxs);
    if (!hth_collision_world_trace_aabb(
            world, body->position, end, mins, maxs, &trace)) {
        return false;
    }
    *out_grounded = trace.hit && !trace.start_solid &&
                    trace.normal.x == 0.0F && trace.normal.y == 1.0F &&
                    trace.normal.z == 0.0F;
    if (*out_grounded) {
        body->position = hth_vec3_add(
            trace.end_position,
            hth_vec3_scale(trace.normal, surface_offset));
        if (body->velocity.y < 0.0F) {
            body->velocity.y = 0.0F;
        }
    }
    return true;
}

static float horizontal_progress_squared(HTHVec3 start, HTHVec3 end)
{
    float x = end.x - start.x;
    float z = end.z - start.z;
    return x * x + z * z;
}

static bool attempt_step(const HTHPlayerBody *start_body,
                         const HTHCollisionWorld *world,
                         float duration,
                         float normal_progress,
                         HTHPlayerBody *out_body,
                         bool *out_accepted)
{
    HTHPlayerBody candidate = *start_body;
    HTHSlideResult slide_result;
    HTHTrace up_trace;
    HTHVec3 maxs;
    HTHVec3 mins;
    HTHVec3 up_end;
    bool grounded;
    float step_progress;

    *out_accepted = false;
    body_extents(&candidate, &mins, &maxs);
    up_end = hth_vec3_add(
        candidate.position, hth_vec3(0.0F, step_height, 0.0F));
    if (!hth_collision_world_trace_aabb(
            world, candidate.position, up_end, mins, maxs, &up_trace)) {
        return false;
    }
    if (up_trace.start_solid || up_trace.hit) {
        return true;
    }
    candidate.position = up_trace.end_position;
    candidate.velocity.y = 0.0F;
    if (!slide_move(&candidate, world, duration, &slide_result)) {
        return false;
    }
    if (slide_result.start_solid ||
        !trace_down_to_ground(&candidate, world,
                              step_height + ground_probe_distance,
                              &grounded) || !grounded) {
        return true;
    }
    step_progress = horizontal_progress_squared(
        start_body->position, candidate.position);
    if (step_progress <= normal_progress + progress_epsilon) {
        return true;
    }
    candidate.grounded = true;
    *out_body = candidate;
    *out_accepted = true;
    return true;
}

bool hth_player_movement_build_intent(const HTHInput *input,
                                      HTHVec3 view_forward,
                                      HTHVec3 view_up,
                                      bool movement_enabled,
                                      HTHPlayerMovementIntent *out_intent)
{
    HTHVec3 horizontal_forward;
    HTHVec3 direction = {0.0F, 0.0F, 0.0F};
    HTHVec3 right;
    float magnitude;

    if (input == NULL || out_intent == NULL) {
        return false;
    }
    out_intent->direction = direction;
    out_intent->magnitude = 0.0F;
    out_intent->jump_pressed = false;
    horizontal_forward = hth_vec3(view_forward.x, 0.0F, view_forward.z);
    if (!hth_vec3_normalize(horizontal_forward, &horizontal_forward) ||
        !hth_vec3_normalize(
            hth_vec3_cross(horizontal_forward, view_up), &right)) {
        return false;
    }
    if (!movement_enabled) {
        return true;
    }
    out_intent->jump_pressed = hth_input_key_pressed(input, HTH_KEY_SPACE);
    if (hth_input_key_down(input, HTH_KEY_W)) {
        direction = hth_vec3_add(direction, horizontal_forward);
    }
    if (hth_input_key_down(input, HTH_KEY_S)) {
        direction = hth_vec3_subtract(direction, horizontal_forward);
    }
    if (hth_input_key_down(input, HTH_KEY_D)) {
        direction = hth_vec3_add(direction, right);
    }
    if (hth_input_key_down(input, HTH_KEY_A)) {
        direction = hth_vec3_subtract(direction, right);
    }
    magnitude = hth_vec3_length(direction);
    if (magnitude > 1.0F) {
        magnitude = 1.0F;
    }
    if (hth_vec3_normalize(direction, &direction)) {
        out_intent->direction = direction;
        out_intent->magnitude = magnitude;
    }
    return true;
}

bool hth_player_movement_step_with_result(
    HTHPlayerBody *body,
    const HTHCollisionWorld *world,
    const HTHMovementConfig *config,
    const HTHPlayerMovementIntent *intent,
    double delta_seconds,
    HTHPlayerMovementResult *out_result)
{
    HTHPlayerBody movement_start;
    HTHPlayerBody normal_result;
    HTHPlayerBody step_result;
    HTHSlideResult slide_result;
    bool grounded;
    bool jump_started;
    bool step_accepted = false;
    bool was_grounded;
    float delta;
    float downward_speed;
    float normal_progress;

    if (!hth_player_body_is_valid(body) ||
        !hth_collision_world_is_valid(world) ||
        !hth_movement_config_is_valid(config) || intent == NULL ||
        !isfinite(delta_seconds) || delta_seconds < 0.0 ||
        out_result == NULL) {
        return false;
    }
    out_result->landed = false;
    out_result->landing_speed = 0.0F;
    was_grounded = body->grounded;
    if (!hth_player_locomotion_update(body, config, intent, delta_seconds,
                                      &jump_started)) {
        return false;
    }
    downward_speed = fmaxf(-body->velocity.y, 0.0F);
    if (delta_seconds == 0.0) {
        return true;
    }
    if (delta_seconds > 0.1) {
        delta_seconds = 0.1;
    }
    delta = (float)delta_seconds;
    movement_start = *body;
    normal_result = *body;
    if (!slide_move(&normal_result, world, delta, &slide_result)) {
        return false;
    }
    normal_progress = horizontal_progress_squared(
        movement_start.position, normal_result.position);
    if (was_grounded && !jump_started &&
        slide_result.blocked_horizontal &&
        (movement_start.velocity.x != 0.0F ||
         movement_start.velocity.z != 0.0F) &&
        !attempt_step(&movement_start, world, delta, normal_progress,
                      &step_result, &step_accepted)) {
        return false;
    }
    *body = step_accepted ? step_result : normal_result;
    if (!step_accepted) {
        float probe_distance = was_grounded && !jump_started
            ? step_height + ground_probe_distance
            : ground_probe_distance;
        if (body->velocity.y > 0.0F) {
            body->grounded = false;
        } else if (!trace_down_to_ground(
                       body, world, probe_distance, &grounded)) {
            return false;
        } else {
            body->grounded = grounded;
        }
    }
    out_result->landed = !was_grounded && body->grounded;
    if (out_result->landed) {
        out_result->landing_speed = downward_speed;
    }
    return true;
}

bool hth_player_movement_step_with_config(
    HTHPlayerBody *body,
    const HTHCollisionWorld *world,
    const HTHMovementConfig *config,
    const HTHPlayerMovementIntent *intent,
    double delta_seconds)
{
    HTHPlayerMovementResult result;

    return hth_player_movement_step_with_result(
        body, world, config, intent, delta_seconds, &result);
}

bool hth_player_movement_step(HTHPlayerBody *body,
                              const HTHCollisionWorld *world,
                              const HTHPlayerMovementIntent *intent,
                              double delta_seconds)
{
    HTHMovementConfig config = hth_movement_config_default();

    return hth_player_movement_step_with_config(
        body, world, &config, intent, delta_seconds);
}
