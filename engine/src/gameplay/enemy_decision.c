#include "enemy_decision.h"

#include "enemy_los.h"
#include "enemy_perception.h"

#include <math.h>

static HTHEnemyIntent idle_intent(void)
{
    HTHEnemyIntent intent = {
        HTH_ENEMY_INTENT_IDLE,
        hth_entity_handle_invalid()
    };

    return intent;
}

bool hth_enemy_decision_evaluate(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    const HTHEnemyTargetStore *targets,
    const HTHSpatialStore *spatial,
    const HTHCollisionWorld *collision_world,
    HTHEntityHandle enemy,
    float perception_radius,
    HTHEnemyIntent *out_intent)
{
    HTHEntityHandle target;

    if (out_intent != NULL) {
        *out_intent = idle_intent();
    }
    if (entities == NULL || actors == NULL || enemies == NULL ||
        targets == NULL || spatial == NULL || collision_world == NULL ||
        out_intent == NULL || !isfinite(perception_radius) ||
        perception_radius < 0.0F ||
        !hth_enemy_store_has(enemies, entities, actors, enemy) ||
        !hth_spatial_store_has(spatial, entities, enemy)) {
        return false;
    }

    if (!hth_enemy_target_store_get(targets, entities, actors, enemies,
                                    enemy, &target) ||
        !hth_enemy_perception_can_perceive(
            entities, actors, enemies, spatial, enemy, target,
            perception_radius) ||
        !hth_enemy_los_has_line_of_sight(
            entities, actors, enemies, spatial, collision_world, enemy,
            target)) {
        return true;
    }

    out_intent->kind = HTH_ENEMY_INTENT_PURSUE;
    out_intent->target = target;
    return true;
}
