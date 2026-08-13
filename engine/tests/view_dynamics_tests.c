#include "view_dynamics.h"

#include <assert.h>
#include <math.h>

static bool close_float(float left, float right, float tolerance)
{
    return fabsf(left - right) <= tolerance;
}

static HTHViewDynamicsInput observation(float eye_y, float speed,
                                        bool grounded)
{
    HTHViewDynamicsInput input;

    input.physical_eye_position = hth_vec3(1.0F, eye_y, 2.0F);
    input.horizontal_speed = speed;
    input.speed_reference = 6.0F;
    input.grounded = grounded;
    input.landed = false;
    input.landing_speed = 0.0F;
    return input;
}

static HTHViewDynamicsOutput update(HTHViewDynamicsState *state,
                                    const HTHViewDynamicsConfig *config,
                                    const HTHViewDynamicsInput *input,
                                    double delta)
{
    HTHViewDynamicsOutput output;

    assert(hth_view_dynamics_update(
        state, config, input, delta, &output));
    return output;
}

static void test_config_validation(void)
{
    HTHViewDynamicsConfig config = hth_view_dynamics_config_default();
    HTHViewDynamicsConfig invalid;

    assert(hth_view_dynamics_config_is_valid(&config));
    invalid = config;
    invalid.step_smoothing_half_life = 0.0F;
    assert(!hth_view_dynamics_config_is_valid(&invalid));
    invalid = config;
    invalid.landing_half_life = -1.0F;
    assert(!hth_view_dynamics_config_is_valid(&invalid));
    invalid = config;
    invalid.fov_half_life = INFINITY;
    assert(!hth_view_dynamics_config_is_valid(&invalid));
    invalid = config;
    invalid.bob_vertical_amplitude = -0.01F;
    assert(!hth_view_dynamics_config_is_valid(&invalid));
    invalid = config;
    invalid.bob_lateral_amplitude = NAN;
    assert(!hth_view_dynamics_config_is_valid(&invalid));
    invalid = config;
    invalid.max_fov_boost_degrees = -1.0F;
    assert(!hth_view_dynamics_config_is_valid(&invalid));
}

static void test_first_frame_has_no_step(void)
{
    HTHViewDynamicsConfig config = hth_view_dynamics_config_default();
    HTHViewDynamicsState state = {0};
    HTHViewDynamicsInput input = observation(100.0F, 0.0F, true);
    HTHViewDynamicsOutput output = update(&state, &config, &input, 0.016);

    assert(state.initialized);
    assert(close_float(state.step_offset_y, 0.0F, 1.0e-6F));
    assert(close_float(output.vertical_offset, 0.0F, 1.0e-6F));
}

static void test_step_up_down_and_decay(void)
{
    HTHViewDynamicsConfig config = hth_view_dynamics_config_default();
    HTHViewDynamicsState up = {0};
    HTHViewDynamicsState down = {0};
    HTHViewDynamicsInput input = observation(1.6F, 0.0F, true);
    HTHViewDynamicsOutput output;
    float previous;
    unsigned int frame;

    (void)update(&up, &config, &input, 0.0);
    input.physical_eye_position.y = 1.8F;
    output = update(&up, &config, &input, 0.0);
    assert(close_float(up.step_offset_y, -0.20F, 1.0e-5F));
    assert(input.physical_eye_position.y + output.vertical_offset < 1.8F);
    previous = up.step_offset_y;
    for (frame = 0; frame < 30; ++frame) {
        output = update(&up, &config, &input, 0.01);
        assert(output.vertical_offset > previous);
        assert(output.vertical_offset <= 0.0F);
        previous = output.vertical_offset;
    }

    input.physical_eye_position.y = 1.8F;
    (void)update(&down, &config, &input, 0.0);
    input.physical_eye_position.y = 1.6F;
    output = update(&down, &config, &input, 0.0);
    assert(close_float(down.step_offset_y, 0.20F, 1.0e-5F));
    assert(input.physical_eye_position.y + output.vertical_offset > 1.6F);
}

static float step_after_time(float delta, unsigned int frames)
{
    HTHViewDynamicsConfig config = hth_view_dynamics_config_default();
    HTHViewDynamicsState state = {0};
    HTHViewDynamicsInput input = observation(1.6F, 0.0F, true);
    unsigned int frame;

    (void)update(&state, &config, &input, 0.0);
    input.physical_eye_position.y = 1.8F;
    (void)update(&state, &config, &input, 0.0);
    for (frame = 0; frame < frames; ++frame) {
        (void)update(&state, &config, &input, (double)delta);
    }
    return state.step_offset_y;
}

static void test_step_frame_rate_and_rejections(void)
{
    HTHViewDynamicsConfig config = hth_view_dynamics_config_default();
    HTHViewDynamicsState state = {0};
    HTHViewDynamicsInput input = observation(1.6F, 0.0F, true);

    assert(close_float(step_after_time(0.01F, 100),
                       step_after_time(0.02F, 50), 1.0e-5F));

    (void)update(&state, &config, &input, 0.0);
    input.physical_eye_position.y = 1.8F;
    input.grounded = false;
    (void)update(&state, &config, &input, 0.0);
    assert(close_float(state.step_offset_y, 0.0F, 1.0e-6F));
    input.physical_eye_position.y = 2.1F;
    (void)update(&state, &config, &input, 0.0);
    assert(close_float(state.step_offset_y, 0.0F, 1.0e-6F));
    input.physical_eye_position.y = 1.6F;
    input.grounded = true;
    input.landed = true;
    input.landing_speed = 1.0F;
    (void)update(&state, &config, &input, 0.0);
    assert(close_float(state.step_offset_y, 0.0F, 1.0e-6F));

    state = (HTHViewDynamicsState){0};
    input = observation(1.6F, 0.0F, true);
    (void)update(&state, &config, &input, 0.0);
    input.physical_eye_position.y = 2.1F;
    (void)update(&state, &config, &input, 0.0);
    assert(close_float(state.step_offset_y, 0.0F, 1.0e-6F));
}

static float landing_after_time(float delta, unsigned int frames)
{
    HTHViewDynamicsConfig config = hth_view_dynamics_config_default();
    HTHViewDynamicsState state = {0};
    HTHViewDynamicsInput input = observation(1.6F, 0.0F, false);
    unsigned int frame;

    (void)update(&state, &config, &input, 0.0);
    input.grounded = true;
    input.landed = true;
    input.landing_speed = 7.0F;
    (void)update(&state, &config, &input, 0.0);
    input.landed = false;
    input.landing_speed = 0.0F;
    for (frame = 0; frame < frames; ++frame) {
        (void)update(&state, &config, &input, (double)delta);
    }
    return state.landing_offset_y;
}

static void test_landing_response_clamp_and_decay(void)
{
    HTHViewDynamicsConfig config = hth_view_dynamics_config_default();
    HTHViewDynamicsState state = {0};
    HTHViewDynamicsInput input = observation(1.6F, 0.0F, false);
    float initial;

    (void)update(&state, &config, &input, 0.0);
    input.grounded = true;
    input.landed = true;
    input.landing_speed = config.min_landing_speed - 0.01F;
    (void)update(&state, &config, &input, 0.0);
    assert(close_float(state.landing_offset_y, 0.0F, 1.0e-6F));

    input.landing_speed = 7.0F;
    (void)update(&state, &config, &input, 0.0);
    assert(state.landing_offset_y < 0.0F);
    initial = state.landing_offset_y;
    input.landed = false;
    input.landing_speed = 0.0F;
    (void)update(&state, &config, &input, 0.1);
    assert(state.landing_offset_y > initial);
    assert(state.landing_offset_y < 0.0F);

    state = (HTHViewDynamicsState){0};
    input = observation(1.6F, 0.0F, false);
    (void)update(&state, &config, &input, 0.0);
    input.grounded = true;
    input.landed = true;
    input.landing_speed = 1000.0F;
    (void)update(&state, &config, &input, 0.0);
    assert(close_float(state.landing_offset_y,
                       -config.max_landing_offset, 1.0e-6F));
    assert(close_float(landing_after_time(0.01F, 100),
                       landing_after_time(0.02F, 50), 1.0e-5F));
}

static void test_bob_behavior(void)
{
    HTHViewDynamicsConfig config = hth_view_dynamics_config_default();
    HTHViewDynamicsState still = {0};
    HTHViewDynamicsState half = {0};
    HTHViewDynamicsState full = {0};
    HTHViewDynamicsInput input = observation(1.6F, 0.0F, true);
    HTHViewDynamicsOutput output;

    output = update(&still, &config, &input, 0.1);
    assert(close_float(output.vertical_offset, 0.0F, 1.0e-6F));
    assert(close_float(output.lateral_offset, 0.0F, 1.0e-6F));

    input.horizontal_speed = 3.0F;
    output = update(&half, &config, &input, 0.1);
    assert(half.bob_phase > 0.0F);
    assert(fabsf(output.vertical_offset) <=
           config.bob_vertical_amplitude * 0.5F + 1.0e-6F);
    input.horizontal_speed = 6.0F;
    output = update(&full, &config, &input, 0.1);
    assert(full.bob_phase > half.bob_phase);
    assert(fabsf(output.vertical_offset) <=
           config.bob_vertical_amplitude + 1.0e-6F);
    assert(fabsf(output.lateral_offset) <=
           config.bob_lateral_amplitude + 1.0e-6F);

    input.horizontal_speed = 600.0F;
    output = update(&full, &config, &input, 0.1);
    assert(fabsf(output.vertical_offset) <=
           config.bob_vertical_amplitude + 1.0e-6F);
    assert(fabsf(output.lateral_offset) <=
           config.bob_lateral_amplitude + 1.0e-6F);
    input.grounded = false;
    output = update(&full, &config, &input, 0.1);
    assert(close_float(full.bob_vertical_offset, 0.0F, 1.0e-6F));
    assert(close_float(full.bob_lateral_offset, 0.0F, 1.0e-6F));
}

static float fov_after_time(float delta, unsigned int frames, float speed)
{
    HTHViewDynamicsConfig config = hth_view_dynamics_config_default();
    HTHViewDynamicsState state = {0};
    HTHViewDynamicsInput input = observation(1.6F, speed, true);
    unsigned int frame;

    for (frame = 0; frame < frames; ++frame) {
        (void)update(&state, &config, &input, (double)delta);
    }
    return state.fov_offset_radians;
}

static void test_fov_response(void)
{
    HTHViewDynamicsConfig config = hth_view_dynamics_config_default();
    HTHViewDynamicsState state = {0};
    HTHViewDynamicsInput input = observation(1.6F, 6.0F, true);
    float maximum = hth_degrees_to_radians(config.max_fov_boost_degrees);
    float moving;

    (void)update(&state, &config, &input, 0.1);
    moving = state.fov_offset_radians;
    assert(moving > 0.0F && moving < maximum);
    input.horizontal_speed = 600.0F;
    (void)update(&state, &config, &input, 10.0);
    assert(state.fov_offset_radians <= maximum + 1.0e-6F);
    input.horizontal_speed = 0.0F;
    (void)update(&state, &config, &input, 0.1);
    assert(state.fov_offset_radians < maximum);
    assert(state.fov_offset_radians > 0.0F);
    assert(close_float(fov_after_time(0.01F, 100, 6.0F),
                       fov_after_time(0.02F, 50, 6.0F), 1.0e-5F));

    state = (HTHViewDynamicsState){0};
    input = observation(1.6F, 6.0F, true);
    input.speed_reference = 0.0F;
    (void)update(&state, &config, &input, 0.1);
    assert(close_float(state.fov_offset_radians, 0.0F, 1.0e-6F));
}

static void test_combined_output_is_component_sum(void)
{
    HTHViewDynamicsConfig config = hth_view_dynamics_config_default();
    HTHViewDynamicsState state = {0};
    HTHViewDynamicsInput input = observation(1.6F, 0.0F, true);
    HTHViewDynamicsOutput output;

    (void)update(&state, &config, &input, 0.0);
    input.physical_eye_position.y = 1.8F;
    input.horizontal_speed = 6.0F;
    input.landed = true;
    input.landing_speed = 7.0F;
    output = update(&state, &config, &input, 0.05);
    assert(close_float(output.vertical_offset,
                       state.step_offset_y + state.landing_offset_y +
                           state.bob_vertical_offset,
                       1.0e-6F));
    assert(close_float(output.lateral_offset,
                       state.bob_lateral_offset, 1.0e-6F));
}

int main(void)
{
    test_config_validation();
    test_first_frame_has_no_step();
    test_step_up_down_and_decay();
    test_step_frame_rate_and_rejections();
    test_landing_response_clamp_and_decay();
    test_bob_behavior();
    test_fov_response();
    test_combined_output_is_component_sum();
    return 0;
}
