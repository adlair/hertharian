#ifndef HTH_VIEW_DYNAMICS_H
#define HTH_VIEW_DYNAMICS_H

#include "hth_math.h"

#include <stdbool.h>

typedef struct {
    float step_smoothing_half_life;
    float max_step_smoothing_delta;
    float min_landing_speed;
    float landing_scale;
    float max_landing_offset;
    float landing_half_life;
    float bob_vertical_amplitude;
    float bob_lateral_amplitude;
    float bob_frequency;
    float max_fov_boost_degrees;
    float fov_half_life;
} HTHViewDynamicsConfig;

typedef struct {
    bool initialized;
    float previous_physical_eye_y;
    bool previous_grounded;
    float step_offset_y;
    float landing_offset_y;
    float bob_phase;
    float bob_vertical_offset;
    float bob_lateral_offset;
    float fov_offset_radians;
} HTHViewDynamicsState;

typedef struct {
    HTHVec3 physical_eye_position;
    float horizontal_speed;
    float speed_reference;
    bool grounded;
    bool landed;
    float landing_speed;
} HTHViewDynamicsInput;

typedef struct {
    float vertical_offset;
    float lateral_offset;
    float fov_offset_radians;
} HTHViewDynamicsOutput;

HTHViewDynamicsConfig hth_view_dynamics_config_default(void);
bool hth_view_dynamics_config_is_valid(
    const HTHViewDynamicsConfig *config);
bool hth_view_dynamics_update(HTHViewDynamicsState *state,
                              const HTHViewDynamicsConfig *config,
                              const HTHViewDynamicsInput *input,
                              double delta_seconds,
                              HTHViewDynamicsOutput *out_output);

#endif
