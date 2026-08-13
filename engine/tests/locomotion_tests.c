#include "player_locomotion.h"

#include <assert.h>
#include <math.h>

static const HTHPlayerMovementIntent no_intent = {
    {0.0F, 0.0F, 0.0F}, 0.0F, false
};
static const HTHPlayerMovementIntent forward_intent = {
    {0.0F, 0.0F, -1.0F}, 1.0F, false
};
static const HTHPlayerMovementIntent right_intent = {
    {1.0F, 0.0F, 0.0F}, 1.0F, false
};
static const HTHPlayerMovementIntent backward_intent = {
    {0.0F, 0.0F, 1.0F}, 1.0F, false
};

static bool close_float(float left, float right, float tolerance)
{
    return fabsf(left - right) <= tolerance;
}

static HTHPlayerBody body_at_origin(bool grounded)
{
    HTHPlayerBody body;

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = grounded;
    return body;
}

static void update(HTHPlayerBody *body, const HTHMovementConfig *config,
                   const HTHPlayerMovementIntent *intent, double delta,
                   bool *jumped)
{
    assert(hth_player_locomotion_update(
        body, config, intent, delta, jumped));
}

static void test_default_and_invalid_configs(void)
{
    HTHMovementConfig config = hth_movement_config_default();
    HTHMovementConfig invalid;

    assert(hth_movement_config_is_valid(&config));
    invalid = config;
    invalid.gravity = 0.0F;
    assert(!hth_movement_config_is_valid(&invalid));
    invalid = config;
    invalid.jump_height = 0.0F;
    assert(!hth_movement_config_is_valid(&invalid));
    invalid = config;
    invalid.ground_acceleration = -1.0F;
    assert(!hth_movement_config_is_valid(&invalid));
    invalid = config;
    invalid.ground_friction = -1.0F;
    assert(!hth_movement_config_is_valid(&invalid));
    invalid = config;
    invalid.max_fall_speed = 0.0F;
    assert(!hth_movement_config_is_valid(&invalid));
}

static void test_ground_friction(void)
{
    HTHMovementConfig config = hth_movement_config_default();
    HTHPlayerBody body = body_at_origin(true);
    bool jumped;
    unsigned int frame;

    body.velocity = hth_vec3(3.0F, 0.25F, -4.0F);
    update(&body, &config, &no_intent, 0.01, &jumped);
    assert(!jumped);
    assert(body.velocity.x > 0.0F && body.velocity.x < 3.0F);
    assert(body.velocity.z < 0.0F && body.velocity.z > -4.0F);
    assert(close_float(body.velocity.x / body.velocity.z,
                       3.0F / -4.0F, 1.0e-4F));
    assert(close_float(body.velocity.y, 0.0F, 1.0e-6F));
    for (frame = 0; frame < 120; ++frame) {
        update(&body, &config, &no_intent, 0.01, &jumped);
    }
    assert(close_float(body.velocity.x, 0.0F, 1.0e-6F));
    assert(close_float(body.velocity.z, 0.0F, 1.0e-6F));
}

static void test_ground_acceleration_turn_and_reverse(void)
{
    HTHMovementConfig config = hth_movement_config_default();
    HTHPlayerBody body = body_at_origin(true);
    bool jumped;
    unsigned int frame;

    update(&body, &config, &forward_intent, 0.01, &jumped);
    assert(body.velocity.z < 0.0F);
    assert(hth_vec3_length(body.velocity) < config.max_ground_speed);
    for (frame = 0; frame < 100; ++frame) {
        update(&body, &config, &forward_intent, 0.01, &jumped);
    }
    assert(body.velocity.z >= -config.max_ground_speed - 1.0e-4F);

    body.velocity = hth_vec3(0.0F, 0.0F, -4.0F);
    update(&body, &config, &right_intent, 0.01, &jumped);
    assert(body.velocity.z < 0.0F);
    assert(body.velocity.x > 0.0F);

    body.velocity = hth_vec3(0.0F, 0.0F, -4.0F);
    update(&body, &config, &backward_intent, 0.01, &jumped);
    assert(body.velocity.z < 0.0F);
    for (frame = 0; frame < 100 && body.velocity.z <= 0.0F; ++frame) {
        update(&body, &config, &backward_intent, 0.01, &jumped);
    }
    assert(body.velocity.z > 0.0F);
}

static void test_world_momentum_and_air_control(void)
{
    HTHMovementConfig config = hth_movement_config_default();
    HTHPlayerBody body = body_at_origin(true);
    bool jumped;

    body.velocity.z = -4.0F;
    update(&body, &config, &right_intent, 0.01, &jumped);
    assert(body.velocity.z < 0.0F);
    assert(body.velocity.x > 0.0F);

    body = body_at_origin(false);
    body.velocity = hth_vec3(1.0F, 0.0F, -2.0F);
    update(&body, &config, &no_intent, 0.02, &jumped);
    assert(close_float(body.velocity.x, 1.0F, 1.0e-6F));
    assert(close_float(body.velocity.z, -2.0F, 1.0e-6F));
    assert(body.velocity.y < 0.0F);
    update(&body, &config, &right_intent, 0.02, &jumped);
    assert(body.velocity.x > 1.0F);
    assert(close_float(body.velocity.z, -2.0F, 1.0e-6F));

    body = body_at_origin(false);
    update(&body, &config, &forward_intent, 0.01, &jumped);
    assert(-body.velocity.z <
           config.ground_acceleration * config.max_ground_speed * 0.01F);
}

static void test_jump_derivation_arc_and_fall_cap(void)
{
    HTHMovementConfig config = hth_movement_config_default();
    HTHPlayerMovementIntent jump = no_intent;
    HTHPlayerBody body = body_at_origin(true);
    float jump_speed;
    float previous_velocity;
    bool crossed_apex = false;
    bool jumped;
    unsigned int frame;

    jump.jump_pressed = true;
    assert(hth_movement_config_jump_speed(&config, &jump_speed));
    assert(close_float(jump_speed * jump_speed,
                       2.0F * fabsf(config.gravity) * config.jump_height,
                       1.0e-4F));
    update(&body, &config, &jump, 0.01, &jumped);
    assert(jumped);
    assert(!body.grounded);
    assert(body.velocity.y > 0.0F);
    previous_velocity = body.velocity.y;
    for (frame = 0; frame < 200; ++frame) {
        update(&body, &config, &jump, 0.01, &jumped);
        assert(!jumped);
        assert(body.velocity.y < previous_velocity);
        if (body.velocity.y <= 0.0F) {
            crossed_apex = true;
            break;
        }
        previous_velocity = body.velocity.y;
    }
    assert(crossed_apex);

    body = body_at_origin(false);
    for (frame = 0; frame < 1000; ++frame) {
        update(&body, &config, &no_intent, 0.1, &jumped);
        assert(body.velocity.y >= -config.max_fall_speed);
    }
    assert(close_float(body.velocity.y, -config.max_fall_speed, 1.0e-5F));
}

static HTHPlayerBody simulate_ground(float delta, unsigned int frames,
                                     const HTHPlayerMovementIntent *intent)
{
    HTHMovementConfig config = hth_movement_config_default();
    HTHPlayerBody body = body_at_origin(true);
    bool jumped;
    unsigned int frame;

    for (frame = 0; frame < frames; ++frame) {
        update(&body, &config, intent, (double)delta, &jumped);
    }
    return body;
}

static float simulate_jump_peak(float delta, unsigned int frames)
{
    HTHMovementConfig config = hth_movement_config_default();
    HTHPlayerMovementIntent jump = no_intent;
    HTHPlayerBody body = body_at_origin(true);
    float peak = 0.0F;
    bool jumped;
    unsigned int frame;

    jump.jump_pressed = true;
    for (frame = 0; frame < frames; ++frame) {
        update(&body, &config, frame == 0 ? &jump : &no_intent,
               (double)delta, &jumped);
        body.position.y += body.velocity.y * delta;
        peak = fmaxf(peak, body.position.y);
    }
    return peak;
}

static void test_frame_rate_robustness(void)
{
    HTHPlayerBody fine = simulate_ground(0.01F, 100, &forward_intent);
    HTHPlayerBody coarse = simulate_ground(0.02F, 50, &forward_intent);
    HTHPlayerBody friction_fine;
    HTHPlayerBody friction_coarse;
    float fine_peak;
    float coarse_peak;

    assert(close_float(fine.velocity.z, coarse.velocity.z, 0.05F));
    friction_fine = body_at_origin(true);
    friction_coarse = body_at_origin(true);
    friction_fine.velocity.x = 6.0F;
    friction_coarse.velocity.x = 6.0F;
    {
        HTHMovementConfig config = hth_movement_config_default();
        bool jumped;
        unsigned int frame;
        for (frame = 0; frame < 100; ++frame) {
            update(&friction_fine, &config, &no_intent, 0.01, &jumped);
        }
        for (frame = 0; frame < 50; ++frame) {
            update(&friction_coarse, &config, &no_intent, 0.02, &jumped);
        }
    }
    assert(close_float(friction_fine.velocity.x,
                       friction_coarse.velocity.x, 0.05F));
    fine_peak = simulate_jump_peak(0.01F, 100);
    coarse_peak = simulate_jump_peak(0.02F, 50);
    assert(close_float(fine_peak, 1.0F, 0.06F));
    assert(close_float(coarse_peak, 1.0F, 0.11F));
    assert(close_float(fine_peak, coarse_peak, 0.06F));
}

int main(void)
{
    test_default_and_invalid_configs();
    test_ground_friction();
    test_ground_acceleration_turn_and_reverse();
    test_world_momentum_and_air_control();
    test_jump_derivation_arc_and_fall_cap();
    test_frame_rate_robustness();
    return 0;
}
