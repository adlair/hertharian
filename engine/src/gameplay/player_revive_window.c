#include "player_revive_window.h"

#include <math.h>
#include <stddef.h>

static bool window_is_valid(const HTHPlayerReviveWindow *window)
{
    if (window == NULL || !isfinite(window->remaining_seconds)) {
        return false;
    }
    if (window->active) {
        return window->remaining_seconds > 0.0;
    }
    return window->remaining_seconds == 0.0;
}

void hth_player_revive_window_reset(HTHPlayerReviveWindow *window)
{
    if (window != NULL) {
        window->remaining_seconds = 0.0;
        window->active = false;
    }
}

bool hth_player_revive_window_begin(
    HTHPlayerReviveWindow *window,
    double duration_seconds)
{
    if (!window_is_valid(window) || window->active ||
        !isfinite(duration_seconds) || duration_seconds <= 0.0) {
        return false;
    }
    window->remaining_seconds = duration_seconds;
    window->active = true;
    return true;
}

bool hth_player_revive_window_advance(
    HTHPlayerReviveWindow *window,
    double delta_seconds,
    bool *out_expired)
{
    if (out_expired != NULL) {
        *out_expired = false;
    }
    if (out_expired == NULL || !window_is_valid(window) ||
        !isfinite(delta_seconds) || delta_seconds < 0.0) {
        return false;
    }
    if (!window->active || delta_seconds == 0.0) {
        return true;
    }
    if (delta_seconds >= window->remaining_seconds) {
        window->remaining_seconds = 0.0;
        window->active = false;
        *out_expired = true;
        return true;
    }
    window->remaining_seconds -= delta_seconds;
    return true;
}

bool hth_player_revive_window_query(
    const HTHPlayerReviveWindow *window,
    bool *out_active,
    double *out_remaining_seconds)
{
    if (out_active != NULL) {
        *out_active = false;
    }
    if (out_remaining_seconds != NULL) {
        *out_remaining_seconds = 0.0;
    }
    if (out_active == NULL || out_remaining_seconds == NULL ||
        !window_is_valid(window)) {
        return false;
    }
    *out_active = window->active;
    *out_remaining_seconds = window->remaining_seconds;
    return true;
}
