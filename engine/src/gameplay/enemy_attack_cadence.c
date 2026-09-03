#include "enemy_attack_cadence.h"

#include <math.h>
#include <stddef.h>

static bool cadence_is_valid(const HTHEnemyAttackCadence *cadence)
{
    return cadence != NULL && isfinite(cadence->remaining_seconds) &&
           cadence->remaining_seconds >= 0.0;
}

void hth_enemy_attack_cadence_reset(HTHEnemyAttackCadence *cadence)
{
    if (cadence != NULL) {
        cadence->remaining_seconds = 0.0;
    }
}

bool hth_enemy_attack_cadence_is_ready(
    const HTHEnemyAttackCadence *cadence,
    bool *out_ready)
{
    if (out_ready != NULL) {
        *out_ready = false;
    }
    if (out_ready == NULL || !cadence_is_valid(cadence)) {
        return false;
    }

    *out_ready = cadence->remaining_seconds == 0.0;
    return true;
}

bool hth_enemy_attack_cadence_advance(
    HTHEnemyAttackCadence *cadence,
    double delta_seconds)
{
    double remaining;

    if (!cadence_is_valid(cadence) || !isfinite(delta_seconds) ||
        delta_seconds < 0.0) {
        return false;
    }

    remaining = cadence->remaining_seconds;
    cadence->remaining_seconds = delta_seconds >= remaining
        ? 0.0
        : remaining - delta_seconds;
    return true;
}

bool hth_enemy_attack_cadence_commit(
    HTHEnemyAttackCadence *cadence,
    double interval_seconds)
{
    if (!cadence_is_valid(cadence) || !isfinite(interval_seconds) ||
        interval_seconds < 0.0 || cadence->remaining_seconds != 0.0) {
        return false;
    }

    cadence->remaining_seconds = interval_seconds;
    return true;
}
