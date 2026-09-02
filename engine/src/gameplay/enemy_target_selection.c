#include "enemy_target_selection.h"

#include "enemy_los.h"
#include "enemy_perception.h"

#include <math.h>

static double distance_squared(HTHVec3 left, HTHVec3 right)
{
    const double dx = (double)right.x - (double)left.x;
    const double dy = (double)right.y - (double)left.y;
    const double dz = (double)right.z - (double)left.z;

    return dx * dx + dy * dy + dz * dz;
}

bool hth_enemy_target_select(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    const HTHSpatialStore *spatial,
    const HTHCollisionWorld *collision_world,
    HTHEnemyTargetStore *targets,
    HTHEntityHandle enemy,
    const HTHEntityHandle *candidates,
    size_t candidate_count,
    float perception_radius,
    HTHEntityHandle *out_selected)
{
    HTHSpatialTransform enemy_transform;
    HTHEntityHandle best = hth_entity_handle_invalid();
    double best_distance_squared = 0.0;
    bool found = false;
    size_t index;

    if (out_selected != NULL) {
        *out_selected = hth_entity_handle_invalid();
    }
    if (entities == NULL || actors == NULL || enemies == NULL ||
        spatial == NULL || collision_world == NULL || targets == NULL ||
        out_selected == NULL ||
        (candidate_count > 0U && candidates == NULL) ||
        !isfinite(perception_radius) || perception_radius < 0.0F ||
        !hth_enemy_store_has(enemies, entities, actors, enemy) ||
        !hth_spatial_store_get(spatial, entities, enemy, &enemy_transform)) {
        return false;
    }

    for (index = 0U; index < candidate_count; ++index) {
        const HTHEntityHandle candidate = candidates[index];
        HTHSpatialTransform candidate_transform;
        double candidate_distance_squared;

        if (hth_entity_handle_equal(candidate, enemy) ||
            !hth_spatial_store_get(spatial, entities, candidate,
                                   &candidate_transform) ||
            !hth_enemy_perception_can_perceive(
                entities, actors, enemies, spatial, enemy, candidate,
                perception_radius) ||
            !hth_enemy_los_has_line_of_sight(
                entities, actors, enemies, spatial, collision_world, enemy,
                candidate)) {
            continue;
        }
        candidate_distance_squared = distance_squared(
            enemy_transform.position, candidate_transform.position);
        if (!found || candidate_distance_squared < best_distance_squared ||
            (candidate_distance_squared == best_distance_squared &&
             candidate.index < best.index)) {
            found = true;
            best = candidate;
            best_distance_squared = candidate_distance_squared;
        }
    }

    if (!found) {
        return true;
    }
    if (!hth_enemy_target_store_set(targets, entities, actors, enemies,
                                    enemy, best)) {
        return false;
    }
    *out_selected = best;
    return true;
}
