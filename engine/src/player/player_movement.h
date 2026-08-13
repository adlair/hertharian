#ifndef HTH_PLAYER_MOVEMENT_H
#define HTH_PLAYER_MOVEMENT_H

#include "collision_world.h"
#include "hth_input.h"
#include "hth_math.h"
#include "player_body.h"
#include "player_locomotion.h"

#include <stdbool.h>

bool hth_player_movement_build_intent(const HTHInput *input,
                                      HTHVec3 view_forward,
                                      HTHVec3 view_up,
                                      bool movement_enabled,
                                      HTHPlayerMovementIntent *out_intent);
bool hth_player_movement_step(HTHPlayerBody *body,
                              const HTHCollisionWorld *world,
                              const HTHPlayerMovementIntent *intent,
                              double delta_seconds);
bool hth_player_movement_step_with_config(
    HTHPlayerBody *body,
    const HTHCollisionWorld *world,
    const HTHMovementConfig *config,
    const HTHPlayerMovementIntent *intent,
    double delta_seconds);

#endif
