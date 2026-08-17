#include "damage_intent.h"

#include <math.h>

bool hth_damage_intent_is_valid(const HTHDamageIntent *intent,
                                const HTHEntityRegistry *entities,
                                const HTHActorStore *actors)
{
    if (intent == NULL || entities == NULL || actors == NULL ||
        !isfinite(intent->amount) || intent->amount < 0.0F) {
        return false;
    }
    return hth_actor_store_has(actors, entities, intent->source) &&
           hth_actor_store_has(actors, entities, intent->target);
}

bool hth_damage_intent_resolve(const HTHDamageIntent *intent,
                               const HTHEntityRegistry *entities,
                               const HTHActorStore *actors,
                               HTHHealthStore *health,
                               HTHDamageResolution *out_result)
{
    HTHDamageResult damage;

    if (out_result != NULL) {
        *out_result = (HTHDamageResolution){0};
    }
    if (out_result == NULL || health == NULL ||
        !hth_damage_intent_is_valid(intent, entities, actors)) {
        return false;
    }
    if (!hth_health_store_has(health, entities, actors, intent->target)) {
        return true;
    }
    if (!hth_health_store_apply_damage(health, entities, actors,
                                       intent->target, intent->amount,
                                       &damage)) {
        return false;
    }
    out_result->damage = damage;
    out_result->applied = true;
    return true;
}
