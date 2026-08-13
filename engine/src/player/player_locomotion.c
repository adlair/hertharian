#include "player_locomotion.h"

#include <math.h>
#include <stddef.h>

static const double maximum_delta_seconds = 0.1;
static const float speed_epsilon = 1.0e-6F;

HTHMovementConfig hth_movement_config_default(void)
{
    HTHMovementConfig config;

    config.max_ground_speed = 6.0F;
    config.ground_acceleration = 12.0F;
    config.ground_friction = 7.0F;
    config.stop_speed = 1.5F;
    config.air_acceleration = 2.5F;
    config.max_air_wish_speed = 3.0F;
    config.gravity = -9.81F;
    config.jump_height = 1.0F;
    config.max_fall_speed = 40.0F;
    return config;
}

bool hth_movement_config_is_valid(const HTHMovementConfig *config)
{
    return config != NULL && isfinite(config->max_ground_speed) &&
           config->max_ground_speed >= 0.0F &&
           isfinite(config->ground_acceleration) &&
           config->ground_acceleration >= 0.0F &&
           isfinite(config->ground_friction) &&
           config->ground_friction >= 0.0F &&
           isfinite(config->stop_speed) && config->stop_speed >= 0.0F &&
           isfinite(config->air_acceleration) &&
           config->air_acceleration >= 0.0F &&
           isfinite(config->max_air_wish_speed) &&
           config->max_air_wish_speed >= 0.0F &&
           isfinite(config->gravity) && config->gravity < 0.0F &&
           isfinite(config->jump_height) && config->jump_height > 0.0F &&
           isfinite(config->max_fall_speed) &&
           config->max_fall_speed > 0.0F;
}

bool hth_movement_config_jump_speed(const HTHMovementConfig *config,
                                    float *out_jump_speed)
{
    if (!hth_movement_config_is_valid(config) || out_jump_speed == NULL) {
        return false;
    }
    *out_jump_speed = sqrtf(
        2.0F * fabsf(config->gravity) * config->jump_height);
    return isfinite(*out_jump_speed);
}

static bool intent_is_valid(const HTHPlayerMovementIntent *intent)
{
    float length;

    if (intent == NULL || !isfinite(intent->direction.x) ||
        !isfinite(intent->direction.y) ||
        !isfinite(intent->direction.z) ||
        !isfinite(intent->magnitude) || intent->magnitude < 0.0F ||
        intent->magnitude > 1.0F || intent->direction.y != 0.0F) {
        return false;
    }
    length = hth_vec3_length(intent->direction);
    return (intent->magnitude == 0.0F && length <= speed_epsilon) ||
           (intent->magnitude > 0.0F &&
            fabsf(length - 1.0F) <= 1.0e-4F);
}

static void apply_ground_friction(HTHPlayerBody *body,
                                  const HTHMovementConfig *config,
                                  float delta)
{
    float speed = sqrtf(body->velocity.x * body->velocity.x +
                        body->velocity.z * body->velocity.z);
    float control;
    float new_speed;

    if (speed <= speed_epsilon) {
        body->velocity.x = 0.0F;
        body->velocity.z = 0.0F;
        return;
    }
    control = fmaxf(speed, config->stop_speed);
    new_speed = fmaxf(
        0.0F, speed - control * config->ground_friction * delta);
    body->velocity.x *= new_speed / speed;
    body->velocity.z *= new_speed / speed;
}

static void accelerate(HTHPlayerBody *body, HTHVec3 wish_direction,
                       float wish_speed, float acceleration, float delta)
{
    float current_speed;
    float add_speed;
    float acceleration_speed;

    if (wish_speed <= 0.0F) {
        return;
    }
    current_speed = body->velocity.x * wish_direction.x +
                    body->velocity.z * wish_direction.z;
    add_speed = wish_speed - current_speed;
    if (add_speed <= 0.0F) {
        return;
    }
    acceleration_speed = acceleration * wish_speed * delta;
    acceleration_speed = fminf(acceleration_speed, add_speed);
    body->velocity.x += wish_direction.x * acceleration_speed;
    body->velocity.z += wish_direction.z * acceleration_speed;
}

bool hth_player_locomotion_update(HTHPlayerBody *body,
                                  const HTHMovementConfig *config,
                                  const HTHPlayerMovementIntent *intent,
                                  double delta_seconds,
                                  bool *out_jump_started)
{
    bool was_grounded;
    float delta;
    float wish_speed;

    if (!hth_player_body_is_valid(body) ||
        !hth_movement_config_is_valid(config) ||
        !intent_is_valid(intent) || !isfinite(delta_seconds) ||
        delta_seconds < 0.0 || out_jump_started == NULL) {
        return false;
    }
    *out_jump_started = false;
    if (delta_seconds == 0.0) {
        return true;
    }
    if (delta_seconds > maximum_delta_seconds) {
        delta_seconds = maximum_delta_seconds;
    }
    delta = (float)delta_seconds;
    was_grounded = body->grounded;
    wish_speed = config->max_ground_speed * intent->magnitude;

    if (was_grounded) {
        float jump_speed;

        body->velocity.y = 0.0F;
        apply_ground_friction(body, config, delta);
        accelerate(body, intent->direction, wish_speed,
                   config->ground_acceleration, delta);
        if (intent->jump_pressed) {
            if (!hth_movement_config_jump_speed(config, &jump_speed)) {
                return false;
            }
            body->velocity.y = jump_speed;
            body->grounded = false;
            *out_jump_started = true;
        }
    } else {
        wish_speed = fminf(wish_speed, config->max_air_wish_speed);
        accelerate(body, intent->direction, wish_speed,
                   config->air_acceleration, delta);
    }

    if (!was_grounded || *out_jump_started) {
        body->velocity.y += config->gravity * delta;
        body->velocity.y = fmaxf(body->velocity.y,
                                 -config->max_fall_speed);
    }
    return hth_player_body_is_valid(body);
}
