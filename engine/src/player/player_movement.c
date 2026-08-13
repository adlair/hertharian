#include "player_movement.h"

#include <math.h>

static const float move_speed = 4.0F;
static const float gravity = -9.81F;
static const double maximum_delta_seconds = 0.1;

bool hth_player_movement_build_intent(const HTHInput *input,
                                      HTHVec3 view_forward,
                                      HTHVec3 view_up,
                                      bool movement_enabled,
                                      HTHPlayerMovementIntent *out_intent)
{
    HTHVec3 horizontal_forward;
    HTHVec3 direction = {0.0F, 0.0F, 0.0F};
    HTHVec3 right;

    if (input == NULL || out_intent == NULL) {
        return false;
    }
    out_intent->direction = direction;
    horizontal_forward = hth_vec3(view_forward.x, 0.0F, view_forward.z);
    if (!hth_vec3_normalize(horizontal_forward, &horizontal_forward) ||
        !hth_vec3_normalize(
            hth_vec3_cross(horizontal_forward, view_up), &right)) {
        return false;
    }
    if (!movement_enabled) {
        return true;
    }
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
    if (hth_vec3_normalize(direction, &direction)) {
        out_intent->direction = direction;
    }
    return true;
}

bool hth_player_movement_step(HTHPlayerBody *body,
                              const HTHCollisionWorld *world,
                              const HTHPlayerMovementIntent *intent,
                              double delta_seconds)
{
    float delta;
    HTHVec3 displacement;

    if (!hth_player_body_is_valid(body) ||
        !hth_collision_world_is_valid(world) || intent == NULL ||
        !isfinite(intent->direction.x) ||
        !isfinite(intent->direction.y) ||
        !isfinite(intent->direction.z) || !isfinite(delta_seconds) ||
        delta_seconds < 0.0) {
        return false;
    }
    body->velocity.x = intent->direction.x * move_speed;
    body->velocity.z = intent->direction.z * move_speed;
    if (delta_seconds == 0.0) {
        return true;
    }
    if (delta_seconds > maximum_delta_seconds) {
        delta_seconds = maximum_delta_seconds;
    }
    delta = (float)delta_seconds;
    body->velocity.y += gravity * delta;
    displacement = hth_vec3_scale(body->velocity, delta);
    return hth_collision_world_move_body(world, body, displacement);
}
