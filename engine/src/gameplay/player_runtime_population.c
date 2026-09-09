#include "player_runtime_population.h"

#include "actor_spawn.h"

#include <stddef.h>

bool hth_player_runtime_spawn(
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    HTHPlayerLifecycleRuntime *lifecycle,
    const HTHPlayerRuntimeSpawnSpec *spec,
    HTHPlayerSlot *out_slot)
{
    HTHActorSpawnSpec actor_spec = {0};
    HTHEntityHandle player;
    HTHPlayerSlot slot;

    if (out_slot != NULL) {
        *out_slot = HTH_PLAYER_SLOT_INVALID;
    }
    if (entities == NULL || actors == NULL || spatial == NULL ||
        bodies == NULL || health == NULL || lifecycle == NULL ||
        spec == NULL || out_slot == NULL ||
        !hth_health_is_valid(spec->health)) {
        return false;
    }

    actor_spec.has_spatial = true;
    actor_spec.transform = spec->transform;
    actor_spec.has_body = false;
    actor_spec.has_health = true;
    actor_spec.health = spec->health;
    if (!hth_actor_spawn(entities, actors, spatial, bodies, health,
                         &actor_spec, &player)) {
        return false;
    }
    if (!hth_player_lifecycle_runtime_register(
            lifecycle, entities, actors, player, &slot)) {
        (void)hth_actor_despawn(entities, actors, spatial, bodies, health,
                                player);
        return false;
    }

    *out_slot = slot;
    return true;
}

bool hth_player_runtime_despawn(
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    HTHPlayerLifecycleRuntime *lifecycle,
    HTHPlayerSlot slot)
{
    HTHEntityHandle player;
    const HTHPlayerRoster *roster;

    if (entities == NULL || actors == NULL || spatial == NULL ||
        bodies == NULL || health == NULL || lifecycle == NULL ||
        slot >= HTH_MAX_PLAYERS) {
        return false;
    }
    roster = hth_player_lifecycle_runtime_get_roster(lifecycle);
    if (roster == NULL ||
        !hth_player_roster_get_slot(roster, entities, actors, slot,
                                    &player) ||
        !hth_spatial_store_has(spatial, entities, player) ||
        !hth_health_store_has(health, entities, actors, player)) {
        return false;
    }
    if (!hth_player_lifecycle_runtime_unregister(lifecycle, slot)) {
        return false;
    }
    return hth_actor_despawn(entities, actors, spatial, bodies, health,
                             player);
}
