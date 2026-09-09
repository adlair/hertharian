#include "player_revive_config.h"

#include <math.h>
#include <stddef.h>

bool hth_player_revive_config_is_valid(
    const HTHPlayerReviveConfig *config)
{
    return config != NULL &&
           isfinite(config->revive_window_duration_seconds) &&
           config->revive_window_duration_seconds > 0.0 &&
           isfinite(config->revive_range) && config->revive_range > 0.0F &&
           isfinite(config->hold_duration_seconds) &&
           config->hold_duration_seconds > 0.0 &&
           isfinite(config->revive_health) && config->revive_health > 0.0F;
}

HTHPlayerReviveConfig hth_player_revive_config_default(void)
{
    const HTHPlayerReviveConfig config = {
        10.0,
        2.0F,
        2.0,
        25.0F
    };

    return config;
}
