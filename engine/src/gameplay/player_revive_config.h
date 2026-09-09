#ifndef HTH_PLAYER_REVIVE_CONFIG_H
#define HTH_PLAYER_REVIVE_CONFIG_H

#include <stdbool.h>

typedef struct HTHPlayerReviveConfig {
    double revive_window_duration_seconds;
    float revive_range;
    double hold_duration_seconds;
    float revive_health;
} HTHPlayerReviveConfig;

bool hth_player_revive_config_is_valid(
    const HTHPlayerReviveConfig *config);
HTHPlayerReviveConfig hth_player_revive_config_default(void);

#endif
