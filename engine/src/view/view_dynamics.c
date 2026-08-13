#include "view_dynamics.h"

#include <math.h>
#include <stddef.h>

static const double maximum_delta_seconds = 0.1;
static const float speed_epsilon = 1.0e-6F;
static const float two_pi = 6.28318530717958647692F;
static const float four_pi = 12.56637061435917295384F;

HTHViewDynamicsConfig hth_view_dynamics_config_default(void)
{
    HTHViewDynamicsConfig config;

    config.step_smoothing_half_life = 0.08F;
    config.max_step_smoothing_delta = 0.35F;
    config.min_landing_speed = 1.5F;
    config.landing_scale = 0.035F;
    config.max_landing_offset = 0.14F;
    config.landing_half_life = 0.16F;
    config.bob_vertical_amplitude = 0.035F;
    config.bob_lateral_amplitude = 0.014F;
    config.bob_frequency = 1.7F;
    config.max_fov_boost_degrees = 4.0F;
    config.fov_half_life = 0.12F;
    return config;
}

bool hth_view_dynamics_config_is_valid(
    const HTHViewDynamicsConfig *config)
{
    return config != NULL &&
           isfinite(config->step_smoothing_half_life) &&
           config->step_smoothing_half_life > 0.0F &&
           isfinite(config->max_step_smoothing_delta) &&
           config->max_step_smoothing_delta >= 0.0F &&
           isfinite(config->min_landing_speed) &&
           config->min_landing_speed >= 0.0F &&
           isfinite(config->landing_scale) &&
           config->landing_scale >= 0.0F &&
           isfinite(config->max_landing_offset) &&
           config->max_landing_offset >= 0.0F &&
           isfinite(config->landing_half_life) &&
           config->landing_half_life > 0.0F &&
           isfinite(config->bob_vertical_amplitude) &&
           config->bob_vertical_amplitude >= 0.0F &&
           isfinite(config->bob_lateral_amplitude) &&
           config->bob_lateral_amplitude >= 0.0F &&
           isfinite(config->bob_frequency) &&
           config->bob_frequency >= 0.0F &&
           isfinite(config->max_fov_boost_degrees) &&
           config->max_fov_boost_degrees >= 0.0F &&
           isfinite(config->fov_half_life) &&
           config->fov_half_life > 0.0F;
}

static bool input_is_valid(const HTHViewDynamicsInput *input)
{
    return input != NULL && isfinite(input->physical_eye_position.x) &&
           isfinite(input->physical_eye_position.y) &&
           isfinite(input->physical_eye_position.z) &&
           isfinite(input->horizontal_speed) &&
           input->horizontal_speed >= 0.0F &&
           isfinite(input->speed_reference) &&
           input->speed_reference >= 0.0F &&
           isfinite(input->landing_speed) && input->landing_speed >= 0.0F;
}

static float half_life_decay(float delta, float half_life)
{
    return exp2f(-delta / half_life);
}

static float speed_normalized(const HTHViewDynamicsInput *input)
{
    if (input->speed_reference <= speed_epsilon) {
        return 0.0F;
    }
    return fminf(input->horizontal_speed / input->speed_reference, 1.0F);
}

bool hth_view_dynamics_update(HTHViewDynamicsState *state,
                              const HTHViewDynamicsConfig *config,
                              const HTHViewDynamicsInput *input,
                              double delta_seconds,
                              HTHViewDynamicsOutput *out_output)
{
    float delta;
    float decay;
    float normalized_speed;
    float target_fov;

    if (state == NULL || !hth_view_dynamics_config_is_valid(config) ||
        !input_is_valid(input) || !isfinite(delta_seconds) ||
        delta_seconds < 0.0 || out_output == NULL) {
        return false;
    }
    if (delta_seconds > maximum_delta_seconds) {
        delta_seconds = maximum_delta_seconds;
    }
    delta = (float)delta_seconds;

    if (!state->initialized) {
        state->initialized = true;
        state->previous_physical_eye_y = input->physical_eye_position.y;
        state->previous_grounded = input->grounded;
        state->step_offset_y = 0.0F;
        state->landing_offset_y = 0.0F;
        state->bob_phase = 0.0F;
        state->bob_vertical_offset = 0.0F;
        state->bob_lateral_offset = 0.0F;
        state->fov_offset_radians = 0.0F;
    } else {
        float eye_delta = input->physical_eye_position.y -
                          state->previous_physical_eye_y;

        if (state->previous_grounded && input->grounded &&
            fabsf(eye_delta) <= config->max_step_smoothing_delta) {
            state->step_offset_y -= eye_delta;
        }
        state->previous_physical_eye_y = input->physical_eye_position.y;
        state->previous_grounded = input->grounded;
    }

    if (input->landed && input->landing_speed >= config->min_landing_speed) {
        float magnitude = (input->landing_speed -
                           config->min_landing_speed) *
                          config->landing_scale;
        magnitude = fminf(magnitude, config->max_landing_offset);
        state->landing_offset_y -= magnitude;
    }

    state->step_offset_y *= half_life_decay(
        delta, config->step_smoothing_half_life);
    state->landing_offset_y *= half_life_decay(
        delta, config->landing_half_life);

    normalized_speed = speed_normalized(input);
    if (input->grounded && normalized_speed > 0.0F) {
        state->bob_phase += two_pi * config->bob_frequency *
                            normalized_speed * delta;
        state->bob_phase = fmodf(state->bob_phase, four_pi);
        state->bob_vertical_offset = sinf(state->bob_phase) *
            config->bob_vertical_amplitude * normalized_speed;
        state->bob_lateral_offset = sinf(state->bob_phase * 0.5F) *
            config->bob_lateral_amplitude * normalized_speed;
    } else {
        state->bob_vertical_offset = 0.0F;
        state->bob_lateral_offset = 0.0F;
    }

    target_fov = hth_degrees_to_radians(
        config->max_fov_boost_degrees * normalized_speed);
    decay = half_life_decay(delta, config->fov_half_life);
    state->fov_offset_radians = target_fov +
        (state->fov_offset_radians - target_fov) * decay;

    out_output->vertical_offset = state->step_offset_y +
        state->landing_offset_y + state->bob_vertical_offset;
    out_output->lateral_offset = state->bob_lateral_offset;
    out_output->fov_offset_radians = state->fov_offset_radians;
    return isfinite(out_output->vertical_offset) &&
           isfinite(out_output->lateral_offset) &&
           isfinite(out_output->fov_offset_radians);
}
