#ifndef HTH_DAMAGE_INTENT_H
#define HTH_DAMAGE_INTENT_H

#include "actor.h"
#include "entity.h"
#include "health.h"

#include <stdbool.h>

typedef struct {
    HTHEntityHandle source;
    HTHEntityHandle target;
    float amount;
} HTHDamageIntent;

typedef struct {
    bool applied;
    HTHDamageResult damage;
} HTHDamageResolution;

bool hth_damage_intent_is_valid(const HTHDamageIntent *intent,
                                const HTHEntityRegistry *entities,
                                const HTHActorStore *actors);
bool hth_damage_intent_resolve(const HTHDamageIntent *intent,
                               const HTHEntityRegistry *entities,
                               const HTHActorStore *actors,
                               HTHHealthStore *health,
                               HTHDamageResolution *out_result);

#endif
