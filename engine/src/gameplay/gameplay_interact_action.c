#include "gameplay_interact_action.h"

#include <stddef.h>

static bool binding_is_valid(HTHKey binding)
{
    return binding > HTH_KEY_UNKNOWN && binding < HTH_KEY_COUNT;
}

bool hth_gameplay_interact_action_query(
    const HTHInput *input,
    HTHKey binding,
    HTHGameplayActionState *out_state)
{
    HTHGameplayActionState state = {false, false, false};

    if (out_state != NULL) {
        *out_state = state;
    }
    if (input == NULL || out_state == NULL ||
        !binding_is_valid(binding)) {
        return false;
    }

    state.down = hth_input_key_down(input, binding);
    state.pressed = hth_input_key_pressed(input, binding);
    state.released = hth_input_key_released(input, binding);
    *out_state = state;
    return true;
}
