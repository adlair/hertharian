#ifndef HTH_ENEMY_ATTACK_CADENCE_H
#define HTH_ENEMY_ATTACK_CADENCE_H

#include <stdbool.h>

typedef struct HTHEnemyAttackCadence {
    double remaining_seconds;
} HTHEnemyAttackCadence;

void hth_enemy_attack_cadence_reset(HTHEnemyAttackCadence *cadence);
bool hth_enemy_attack_cadence_is_ready(
    const HTHEnemyAttackCadence *cadence,
    bool *out_ready);
bool hth_enemy_attack_cadence_advance(
    HTHEnemyAttackCadence *cadence,
    double delta_seconds);
bool hth_enemy_attack_cadence_commit(
    HTHEnemyAttackCadence *cadence,
    double interval_seconds);

#endif
