#include "enemy_runtime_population.h"

#include "actor_spawn.h"

bool hth_enemy_runtime_spawn(
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHEnemyStore *enemies,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    const HTHEnemyRuntimeSpawnSpec *spec,
    HTHEntityHandle *out_enemy)
{
    HTHActorSpawnSpec actor_spec;
    HTHEntityHandle enemy;

    if (out_enemy != NULL) {
        *out_enemy = hth_entity_handle_invalid();
    }
    if (entities == NULL || actors == NULL || enemies == NULL ||
        spatial == NULL || bodies == NULL || health == NULL || spec == NULL ||
        out_enemy == NULL) {
        return false;
    }

    actor_spec.has_spatial = true;
    actor_spec.transform = spec->transform;
    actor_spec.has_body = true;
    actor_spec.body = spec->body;
    actor_spec.has_health = true;
    actor_spec.health = spec->health;

    if (!hth_actor_spawn(entities, actors, spatial, bodies, health,
                         &actor_spec, &enemy)) {
        return false;
    }
    if (!hth_enemy_store_attach(enemies, entities, actors, enemy)) {
        (void)hth_actor_despawn(entities, actors, spatial, bodies, health,
                                enemy);
        return false;
    }
    *out_enemy = enemy;
    return true;
}

bool hth_enemy_runtime_despawn(
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHEnemyStore *enemies,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    HTHEnemyTargetStore *targets,
    HTHEntityHandle enemy)
{
    if (entities == NULL || actors == NULL || enemies == NULL ||
        spatial == NULL || bodies == NULL || health == NULL ||
        targets == NULL ||
        !hth_actor_store_has(actors, entities, enemy) ||
        !hth_enemy_store_has(enemies, entities, actors, enemy)) {
        return false;
    }

    (void)hth_enemy_target_store_clear(targets, entities, enemy);
    if (!hth_enemy_store_remove(enemies, entities, enemy)) {
        return false;
    }
    return hth_actor_despawn(entities, actors, spatial, bodies, health,
                             enemy);
}
