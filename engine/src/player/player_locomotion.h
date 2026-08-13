#ifndef HTH_PLAYER_LOCOMOTION_H
#define HTH_PLAYER_LOCOMOTION_H

#include "hth_math.h"
#include "player_body.h"

#include <stdbool.h>

typedef struct {
    float max_ground_speed;
    float ground_acceleration;
    float ground_friction;
    float stop_speed;
    float air_acceleration;
    float max_air_wish_speed;
    float gravity;
    float jump_height;
    float max_fall_speed;
} HTHMovementConfig;

typedef struct {
    HTHVec3 direction;
    float magnitude;
    bool jump_pressed;
} HTHPlayerMovementIntent;

HTHMovementConfig hth_movement_config_default(void);
bool hth_movement_config_is_valid(const HTHMovementConfig *config);
bool hth_movement_config_jump_speed(const HTHMovementConfig *config,
                                    float *out_jump_speed);
bool hth_player_locomotion_update(HTHPlayerBody *body,
                                  const HTHMovementConfig *config,
                                  const HTHPlayerMovementIntent *intent,
                                  double delta_seconds,
                                  bool *out_jump_started);

#endif
