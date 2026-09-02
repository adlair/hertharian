#include "enemy_attack_eligibility.h"

#include "enemy_los.h"
#include "enemy_perception.h"

#include <math.h>

bool hth_enemy_attack_eligibility_evaluate(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    const HTHSpatialStore *spatial,
    const HTHCollisionWorld *collision_world,
    HTHEntityHandle enemy,
    HTHEntityHandle target,
    float attack_range,
    bool *out_eligible)
{
    if (out_eligible != NULL) {
        *out_eligible = false;
    }
    if (entities == NULL || actors == NULL || enemies == NULL ||
        spatial == NULL || collision_world == NULL || out_eligible == NULL) {
        return false;
    }
    if (!isfinite(attack_range) || attack_range < 0.0F) {
        return false;
    }
    if (!hth_enemy_store_has(enemies, entities, actors, enemy) ||
        !hth_spatial_store_has(spatial, entities, enemy)) {
        return false;
    }
    if (!hth_entity_registry_is_alive(entities, target) ||
        !hth_spatial_store_has(spatial, entities, target)) {
        return false;
    }
    if (hth_entity_handle_equal(enemy, target)) {
        return true;
    }
    if (!hth_enemy_perception_can_perceive(
            entities, actors, enemies, spatial, enemy, target,
            attack_range)) {
        return true;
    }
    if (!hth_enemy_los_has_line_of_sight(
            entities, actors, enemies, spatial, collision_world, enemy,
            target)) {
        return true;
    }
    *out_eligible = true;
    return true;
}
