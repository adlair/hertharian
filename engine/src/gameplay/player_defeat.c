#include "player_defeat.h"

#include <stddef.h>

void hth_player_defeat_reset(HTHPlayerDefeatState *state)
{
    if (state != NULL) {
        state->defeated = false;
    }
}

bool hth_player_defeat_mark(HTHPlayerDefeatState *state)
{
    if (state == NULL) {
        return false;
    }
    state->defeated = true;
    return true;
}

bool hth_player_defeat_is_defeated(
    const HTHPlayerDefeatState *state,
    bool *out_defeated)
{
    if (out_defeated != NULL) {
        *out_defeated = false;
    }
    if (state == NULL || out_defeated == NULL) {
        return false;
    }
    *out_defeated = state->defeated;
    return true;
}
