#ifndef HTH_PLAYER_REVIVE_WINDOW_H
#define HTH_PLAYER_REVIVE_WINDOW_H

#include <stdbool.h>

typedef struct HTHPlayerReviveWindow {
    double remaining_seconds;
    bool active;
} HTHPlayerReviveWindow;

void hth_player_revive_window_reset(HTHPlayerReviveWindow *window);
bool hth_player_revive_window_begin(
    HTHPlayerReviveWindow *window,
    double duration_seconds);
bool hth_player_revive_window_advance(
    HTHPlayerReviveWindow *window,
    double delta_seconds,
    bool *out_expired);
bool hth_player_revive_window_query(
    const HTHPlayerReviveWindow *window,
    bool *out_active,
    double *out_remaining_seconds);

#endif
