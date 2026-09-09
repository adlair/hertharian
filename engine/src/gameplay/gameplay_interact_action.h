#ifndef HTH_GAMEPLAY_INTERACT_ACTION_H
#define HTH_GAMEPLAY_INTERACT_ACTION_H

#include "hth_input.h"

#include <stdbool.h>

typedef struct HTHGameplayActionState {
    bool down;
    bool pressed;
    bool released;
} HTHGameplayActionState;

bool hth_gameplay_interact_action_query(
    const HTHInput *input,
    HTHKey binding,
    HTHGameplayActionState *out_state);

#endif
