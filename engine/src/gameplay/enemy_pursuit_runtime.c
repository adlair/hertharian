#include "enemy_pursuit_runtime.h"

#include "enemy_chase.h"
#include "enemy_decision.h"
#include "enemy_attack_execution.h"
#include "enemy_seek.h"
#include "enemy_target_selection.h"

#include <float.h>
#include <math.h>

bool hth_enemy_pursuit_runtime_step(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    HTHEnemyTargetStore *targets,
    HTHEnemyAttackCadenceStore *cadences,
    const HTHCollisionWorld *collision_world,
    const HTHEntityHandle *candidates,
    size_t candidate_count,
    HTHEntityHandle excluded_target,
    float perception_radius,
    float attack_range,
    float chase_speed,
    float attack_damage,
    double attack_interval_seconds,
    double delta_seconds)
{
    HTHEnemyIterator iterator;
    HTHEntityHandle enemy;

    if (entities == NULL || actors == NULL || enemies == NULL ||
        spatial == NULL || bodies == NULL || health == NULL ||
        targets == NULL || cadences == NULL ||
        !hth_collision_world_is_valid(collision_world) ||
        (candidate_count > 0U && candidates == NULL) ||
        !isfinite(perception_radius) || perception_radius < 0.0F ||
        !isfinite(attack_range) || attack_range < 0.0F ||
        !isfinite(chase_speed) || chase_speed < 0.0F ||
        !isfinite(attack_damage) || attack_damage < 0.0F ||
        !isfinite(attack_interval_seconds) ||
        attack_interval_seconds < 0.0 ||
        !isfinite(delta_seconds) || delta_seconds < 0.0 ||
        delta_seconds > (double)FLT_MAX) {
        return false;
    }

    hth_enemy_iterator_begin(&iterator);
    while (hth_enemy_iterator_next(enemies, entities, actors, &iterator,
                                   &enemy)) {
        HTHEntityHandle target;
        HTHEnemyIntent intent;
        HTHVec3 direction;
        HTHDynamicCollisionResult chase_result;
        HTHEnemyAttackCadence *cadence;
        bool has_target;

        if (!hth_enemy_attack_cadence_store_get_mutable(
                cadences, entities, actors, enemies, enemy, &cadence) ||
            !hth_enemy_attack_cadence_advance(cadence, delta_seconds)) {
            return false;
        }

        has_target = hth_enemy_target_store_get(
            targets, entities, actors, enemies, enemy, &target);
        if (has_target &&
            hth_entity_handle_equal(target, excluded_target)) {
            if (!hth_enemy_target_store_clear(targets, entities, enemy)) {
                return false;
            }
            has_target = false;
        }
        if (!hth_spatial_store_has(spatial, entities, enemy)) {
            continue;
        }
        if (!has_target) {
            HTHEntityHandle selected;

            if (!hth_enemy_target_select(
                    entities, actors, enemies, spatial, collision_world,
                    targets, enemy, candidates, candidate_count,
                    excluded_target, perception_radius, &selected)) {
                return false;
            }
        }
        if (!hth_enemy_target_store_get(
                targets, entities, actors, enemies, enemy, &target)) {
            continue;
        }
        if (!hth_enemy_decision_evaluate_with_attack(
                entities, actors, enemies, targets, spatial,
                collision_world, enemy, perception_radius, attack_range,
                &intent)) {
            return false;
        }
        switch (intent.kind) {
        case HTH_ENEMY_INTENT_IDLE:
            continue;
        case HTH_ENEMY_INTENT_PURSUE:
            break;
        case HTH_ENEMY_INTENT_ATTACK:
        {
            HTHDamageIntent damage_intent;
            HTHDamageResolution resolution;
            bool ready;

            if (!hth_enemy_attack_cadence_is_ready(cadence, &ready)) {
                return false;
            }
            if (!ready) {
                continue;
            }
            if (!hth_enemy_attack_build_damage_intent(
                    entities, actors, enemies, enemy, intent.target,
                    attack_damage, &damage_intent)) {
                return false;
            }
            if (!hth_enemy_attack_cadence_commit(
                    cadence, attack_interval_seconds)) {
                return false;
            }
            if (!hth_damage_intent_resolve(
                    &damage_intent, entities, actors, health,
                    &resolution)) {
                return false;
            }
            continue;
        }
        default:
            return false;
        }
        if (!hth_enemy_seek_compute(
                entities, actors, enemies, spatial, enemy, intent.target,
                &direction)) {
            return false;
        }
        if (!hth_dynamic_body_has(bodies, entities, enemy)) {
            continue;
        }
        if (!hth_enemy_chase_apply(
                entities, actors, enemies, spatial, bodies,
                collision_world, enemy, direction, chase_speed,
                (float)delta_seconds, &chase_result)) {
            return false;
        }
    }
    return true;
}
