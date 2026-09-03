#include "enemy_attack_execution.h"

static HTHDamageIntent invalid_intent(void)
{
    const HTHEntityHandle invalid = hth_entity_handle_invalid();
    const HTHDamageIntent intent = {invalid, invalid, 0.0F};

    return intent;
}

bool hth_enemy_attack_build_damage_intent(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHEntityHandle enemy,
    HTHEntityHandle target,
    float damage,
    HTHDamageIntent *out_intent)
{
    HTHDamageIntent candidate;

    if (out_intent != NULL) {
        *out_intent = invalid_intent();
    }
    if (entities == NULL || actors == NULL || enemies == NULL ||
        out_intent == NULL ||
        !hth_enemy_store_has(enemies, entities, actors, enemy)) {
        return false;
    }

    candidate = (HTHDamageIntent){enemy, target, damage};
    if (!hth_damage_intent_is_valid(&candidate, entities, actors)) {
        return false;
    }

    *out_intent = candidate;
    return true;
}
