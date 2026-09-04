#ifndef HTH_PLAYER_DEFEAT_H
#define HTH_PLAYER_DEFEAT_H

#include <stdbool.h>

typedef struct HTHPlayerDefeatState {
    bool defeated;
} HTHPlayerDefeatState;

void hth_player_defeat_reset(HTHPlayerDefeatState *state);
bool hth_player_defeat_mark(HTHPlayerDefeatState *state);
bool hth_player_defeat_is_defeated(
    const HTHPlayerDefeatState *state,
    bool *out_defeated);

#endif
