#include "enemy_attack_execution.h"

#include "dynamic_body.h"
#include "enemy_decision.h"
#include "enemy_runtime_population.h"
#include "enemy_target.h"
#include "enemy_target_selection.h"
#include "health.h"
#include "player_target_bridge.h"
#include "spatial.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,      \
                    __LINE__, #condition);                                   \
            return false;                                                    \
        }                                                                    \
    } while (0)

typedef struct {
    HTHEntityRegistry *entities;
    HTHActorStore *actors;
    HTHEnemyStore *enemies;
    HTHEnemyAttackCadenceStore *cadences;
    HTHSpatialStore *spatial;
    HTHDynamicBodyStore *bodies;
    HTHHealthStore *health;
    HTHEnemyTargetStore *targets;
} Fixture;

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->enemies = hth_enemy_store_create();
    fixture->cadences = hth_enemy_attack_cadence_store_create();
    fixture->spatial = hth_spatial_store_create();
    fixture->bodies = hth_dynamic_body_store_create();
    fixture->health = hth_health_store_create();
    fixture->targets = hth_enemy_target_store_create();
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->enemies != NULL && fixture->cadences != NULL &&
           fixture->spatial != NULL &&
           fixture->bodies != NULL && fixture->health != NULL &&
           fixture->targets != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_enemy_target_store_destroy(fixture->targets);
    hth_enemy_attack_cadence_store_destroy(fixture->cadences);
    hth_health_store_destroy(fixture->health);
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_spatial_store_destroy(fixture->spatial);
    hth_enemy_store_destroy(fixture->enemies);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static bool handle_is_invalid(HTHEntityHandle handle)
{
    return hth_entity_handle_equal(handle, hth_entity_handle_invalid());
}

static bool intent_is_canonical(HTHDamageIntent intent)
{
    return handle_is_invalid(intent.source) &&
           handle_is_invalid(intent.target) && intent.amount == 0.0F;
}

static bool intent_equals(HTHDamageIntent intent, HTHEntityHandle source,
                          HTHEntityHandle target, float amount)
{
    return hth_entity_handle_equal(intent.source, source) &&
           hth_entity_handle_equal(intent.target, target) &&
           intent.amount == amount;
}

static bool create_entity(Fixture *fixture, HTHEntityHandle *out_entity)
{
    return hth_entity_registry_create_entity(fixture->entities, out_entity);
}

static bool create_actor(Fixture *fixture, HTHEntityHandle *out_actor)
{
    return create_entity(fixture, out_actor) &&
           hth_actor_store_attach(fixture->actors, fixture->entities,
                                  *out_actor);
}

static bool create_enemy(Fixture *fixture, HTHEntityHandle *out_enemy)
{
    return create_actor(fixture, out_enemy) &&
           hth_enemy_store_attach(fixture->enemies, fixture->entities,
                                  fixture->actors, *out_enemy);
}

static bool attach_spatial(Fixture *fixture, HTHEntityHandle entity,
                           float x)
{
    const HTHSpatialTransform transform = {{x, 0.0F, 0.0F}, 0.0F};

    return hth_spatial_store_attach(fixture->spatial, fixture->entities,
                                    entity, &transform);
}

static bool build(const Fixture *fixture, HTHEntityHandle enemy,
                  HTHEntityHandle target, float damage,
                  HTHDamageIntent *out_intent)
{
    return hth_enemy_attack_build_damage_intent(
        fixture->entities, fixture->actors, fixture->enemies, enemy,
        target, damage, out_intent);
}

static bool test_arguments_source_validation_and_canonical_output(void)
{
    Fixture fixture;
    Fixture independent;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle enemy_missing_actor;
    HTHEntityHandle actor_only;
    HTHEntityHandle actor_missing_enemy;
    HTHEntityHandle stale_enemy;
    HTHDamageIntent intent;

    CHECK(fixture_create(&fixture));
    CHECK(fixture_create(&independent));
    CHECK(create_enemy(&fixture, &enemy));
    CHECK(create_actor(&fixture, &target));
    CHECK(create_enemy(&fixture, &enemy_missing_actor));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities,
                                 enemy_missing_actor));
    CHECK(create_actor(&fixture, &actor_only));
    CHECK(create_enemy(&fixture, &actor_missing_enemy));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities,
                                 actor_missing_enemy));
    CHECK(build(&fixture, enemy, target, 3.0F, &intent));
    CHECK(intent_equals(intent, enemy, target, 3.0F));
    CHECK(!hth_enemy_attack_build_damage_intent(
        NULL, fixture.actors, fixture.enemies, enemy, target, 3.0F,
        &intent));
    CHECK(intent_is_canonical(intent));
    intent = (HTHDamageIntent){enemy, target, 3.0F};
    CHECK(!hth_enemy_attack_build_damage_intent(
        fixture.entities, NULL, fixture.enemies, enemy, target, 3.0F,
        &intent));
    CHECK(intent_is_canonical(intent));
    intent = (HTHDamageIntent){enemy, target, 3.0F};
    CHECK(!hth_enemy_attack_build_damage_intent(
        fixture.entities, fixture.actors, NULL, enemy, target, 3.0F,
        &intent));
    CHECK(intent_is_canonical(intent));
    CHECK(!build(&fixture, enemy, target, 3.0F, NULL));
    CHECK(!build(&fixture, hth_entity_handle_invalid(), target, 3.0F,
                 &intent));
    CHECK(intent_is_canonical(intent));
    CHECK(!build(&fixture, enemy_missing_actor, target, 3.0F, &intent));
    CHECK(intent_is_canonical(intent));
    CHECK(!build(&fixture, actor_only, target, 3.0F, &intent));
    CHECK(intent_is_canonical(intent));
    CHECK(!build(&fixture, actor_missing_enemy, target, 3.0F, &intent));
    CHECK(intent_is_canonical(intent));

    stale_enemy = enemy;
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, enemy));
    CHECK(!build(&fixture, stale_enemy, target, 3.0F, &intent));
    CHECK(intent_is_canonical(intent));
    CHECK(create_entity(&fixture, &enemy));
    CHECK(enemy.index == stale_enemy.index);
    CHECK(!build(&fixture, stale_enemy, target, 3.0F, &intent));
    CHECK(intent_is_canonical(intent));
    CHECK(!hth_enemy_attack_build_damage_intent(
        independent.entities, independent.actors, independent.enemies,
        enemy, target, 3.0F, &intent));
    CHECK(intent_is_canonical(intent));
    fixture_destroy(&independent);
    fixture_destroy(&fixture);
    return true;
}

static bool test_target_and_damage_contract(void)
{
    Fixture fixture;
    HTHEntityHandle enemy;
    HTHEntityHandle actor_target;
    HTHEntityHandle other_enemy;
    HTHEntityHandle non_actor;
    HTHEntityHandle stale_target;
    HTHEntityHandle replacement;
    HTHDamageIntent intent;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &enemy));
    CHECK(create_actor(&fixture, &actor_target));
    CHECK(create_enemy(&fixture, &other_enemy));
    CHECK(create_entity(&fixture, &non_actor));
    CHECK(build(&fixture, enemy, actor_target, 0.0F, &intent));
    CHECK(intent_equals(intent, enemy, actor_target, 0.0F));
    CHECK(build(&fixture, enemy, actor_target, 9.25F, &intent));
    CHECK(intent_equals(intent, enemy, actor_target, 9.25F));
    CHECK(hth_damage_intent_is_valid(&intent, fixture.entities,
                                     fixture.actors));
    CHECK(build(&fixture, enemy, other_enemy, 1.0F, &intent));
    CHECK(build(&fixture, enemy, enemy, 1.0F, &intent));
    CHECK(intent_equals(intent, enemy, enemy, 1.0F));
    CHECK(!build(&fixture, enemy, hth_entity_handle_invalid(), 1.0F,
                 &intent));
    CHECK(intent_is_canonical(intent));
    CHECK(!build(&fixture, enemy, non_actor, 1.0F, &intent));
    CHECK(intent_is_canonical(intent));
    CHECK(!build(&fixture, enemy, actor_target, -1.0F, &intent));
    CHECK(intent_is_canonical(intent));
    CHECK(!build(&fixture, enemy, actor_target, NAN, &intent));
    CHECK(intent_is_canonical(intent));
    CHECK(!build(&fixture, enemy, actor_target, INFINITY, &intent));
    CHECK(intent_is_canonical(intent));
    CHECK(!build(&fixture, enemy, actor_target, -INFINITY, &intent));
    CHECK(intent_is_canonical(intent));
    CHECK(build(&fixture, enemy, actor_target, 3.0e30F, &intent));

    for (index = 0U; index < 128U; ++index) {
        CHECK(build(&fixture, enemy, actor_target, 7.5F, &intent));
        CHECK(intent_equals(intent, enemy, actor_target, 7.5F));
    }

    stale_target = actor_target;
    CHECK(hth_entity_registry_destroy_entity(fixture.entities,
                                             actor_target));
    CHECK(!build(&fixture, enemy, stale_target, 1.0F, &intent));
    CHECK(intent_is_canonical(intent));
    CHECK(create_entity(&fixture, &replacement));
    CHECK(replacement.index == stale_target.index);
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 replacement));
    CHECK(!build(&fixture, enemy, stale_target, 1.0F, &intent));
    CHECK(build(&fixture, enemy, replacement, 1.0F, &intent));
    fixture_destroy(&fixture);
    return true;
}

static bool test_optional_state_and_manual_resolution(void)
{
    Fixture fixture;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle target_without_health;
    HTHDamageIntent intent;
    HTHDamageResolution resolution;
    HTHHealth source_before;
    HTHHealth source_after;
    HTHHealth target_before;
    HTHHealth target_after;
    const HTHSpatialTransform source_transform = {{1.0F, 2.0F, 3.0F}, 0.5F};
    HTHSpatialTransform spatial_after;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &enemy));
    CHECK(create_enemy(&fixture, &target));
    CHECK(create_actor(&fixture, &target_without_health));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, enemy,
                                  (HTHHealth){0.0F, 100.0F}));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, target,
                                  (HTHHealth){75.0F, 100.0F}));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   enemy, &source_transform));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, enemy, &source_before));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &target_before));

    CHECK(build(&fixture, enemy, target, 15.0F, &intent));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, enemy, &source_after));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &target_after));
    CHECK(memcmp(&source_before, &source_after, sizeof(source_before)) == 0);
    CHECK(memcmp(&target_before, &target_after, sizeof(target_before)) == 0);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &spatial_after));
    CHECK(memcmp(&source_transform, &spatial_after,
                 sizeof(source_transform)) == 0);
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.applied && resolution.damage.applied == 15.0F);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &target_after));
    CHECK(target_after.current == 60.0F);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, enemy, &source_after));
    CHECK(memcmp(&source_before, &source_after, sizeof(source_before)) == 0);

    CHECK(build(&fixture, enemy, target_without_health, 15.0F, &intent));
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(!resolution.applied && resolution.damage.applied == 0.0F);
    CHECK(build(&fixture, enemy, target, 0.0F, &intent));
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.applied && resolution.damage.applied == 0.0F);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &target_after));
    CHECK(target_after.current == 60.0F);
    fixture_destroy(&fixture);
    return true;
}

static bool test_runtime_enemy_and_zero_health_target_nonmutation(void)
{
    Fixture fixture;
    const HTHEnemyRuntimeSpawnSpec spec = {
        {{4.0F, 5.0F, 6.0F}, 0.25F},
        {{0.25F, 0.5F, 0.25F}, {1.0F, 2.0F, 3.0F}},
        {100.0F, 100.0F}
    };
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHDamageIntent intent;
    HTHSpatialTransform spatial_before;
    HTHSpatialTransform spatial_after;
    HTHDynamicBody body_before;
    HTHDynamicBody body_after;
    HTHHealth enemy_health_before;
    HTHHealth enemy_health_after;
    HTHHealth target_health;
    HTHEntityHandle target_before;
    HTHEntityHandle target_after;
    size_t live_count;

    CHECK(fixture_create(&fixture));
    CHECK(hth_enemy_runtime_spawn(
        fixture.entities, fixture.actors, fixture.enemies, fixture.cadences,
        fixture.spatial,
        fixture.bodies, fixture.health, &spec, &enemy));
    CHECK(create_actor(&fixture, &target));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, target,
                                  (HTHHealth){0.0F, 40.0F}));
    CHECK(hth_enemy_target_store_set(
        fixture.targets, fixture.entities, fixture.actors,
        fixture.enemies, enemy, target));
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors,
        fixture.enemies, enemy, &target_before));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &spatial_before));
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities, enemy,
                               &body_before));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, enemy,
                               &enemy_health_before));
    live_count = hth_entity_registry_live_count(fixture.entities);

    CHECK(build(&fixture, enemy, target, 2.0F, &intent));
    CHECK(intent_equals(intent, enemy, target, 2.0F));
    CHECK(hth_entity_registry_live_count(fixture.entities) == live_count);
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, enemy));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, enemy));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, target));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &spatial_after));
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities, enemy,
                               &body_after));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, enemy,
                               &enemy_health_after));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &target_health));
    CHECK(memcmp(&spatial_before, &spatial_after,
                 sizeof(spatial_before)) == 0);
    CHECK(memcmp(&body_before, &body_after, sizeof(body_before)) == 0);
    CHECK(memcmp(&enemy_health_before, &enemy_health_after,
                 sizeof(enemy_health_before)) == 0);
    CHECK(target_health.current == 0.0F && target_health.maximum == 40.0F);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors,
        fixture.enemies, enemy, &target_after));
    CHECK(hth_entity_handle_equal(target_before, target_after));
    fixture_destroy(&fixture);
    return true;
}

static bool test_player_target_bridge_composition(void)
{
    Fixture fixture;
    HTHPlayerTargetBridge bridge = {hth_entity_handle_invalid()};
    HTHPlayerBody player;
    HTHPlayerBody player_before;
    HTHEntityHandle enemy;
    HTHEntityHandle player_target;
    HTHDamageIntent intent;
    HTHDamageResolution resolution;
    HTHHealth health;
    HTHSpatialTransform spatial_before;
    HTHSpatialTransform spatial_after;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &enemy));
    CHECK(hth_player_body_init(&player, (HTHVec3){2.0F, 0.0F, 0.0F}));
    player_before = player;
    CHECK(hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, fixture.health, &player,
        (HTHHealth){100.0F, 100.0F}));
    CHECK(hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &player_target));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                player_target, &spatial_before));
    CHECK(build(&fixture, enemy, player_target, 12.0F, &intent));
    CHECK(memcmp(&player, &player_before, sizeof(player)) == 0);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, player_target, &health));
    CHECK(health.current == 100.0F);
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.applied && resolution.damage.applied == 12.0F);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, player_target, &health));
    CHECK(health.current == 88.0F);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                player_target, &spatial_after));
    CHECK(memcmp(&spatial_before, &spatial_after,
                 sizeof(spatial_before)) == 0);
    CHECK(!hth_enemy_store_has(fixture.enemies, fixture.entities,
                               fixture.actors, player_target));
    CHECK(!hth_enemy_target_store_has(
        fixture.targets, fixture.entities, fixture.actors,
        fixture.enemies, enemy));
    fixture_destroy(&fixture);
    return true;
}

static bool test_decision_and_selection_composition(void)
{
    Fixture fixture;
    const HTHCollisionWorld clear_world = {0};
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle selected;
    HTHEnemyIntent decision;
    HTHDamageIntent damage_intent;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &enemy));
    CHECK(create_actor(&fixture, &target));
    CHECK(attach_spatial(&fixture, enemy, 0.0F));
    CHECK(attach_spatial(&fixture, target, 1.0F));
    CHECK(hth_enemy_target_select(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        &clear_world, fixture.targets, enemy, &target, 1U, 10.0F,
        &selected));
    CHECK(hth_entity_handle_equal(selected, target));
    CHECK(hth_enemy_decision_evaluate_with_attack(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &clear_world, enemy, 10.0F, 1.0F,
        &decision));
    CHECK(decision.kind == HTH_ENEMY_INTENT_ATTACK);
    CHECK(hth_entity_handle_equal(decision.target, target));
    CHECK(build(&fixture, enemy, decision.target, 5.0F, &damage_intent));
    CHECK(intent_equals(damage_intent, enemy, target, 5.0F));
    CHECK(hth_enemy_target_store_has(
        fixture.targets, fixture.entities, fixture.actors,
        fixture.enemies, enemy));
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_arguments_source_validation_and_canonical_output,
        test_target_and_damage_contract,
        test_optional_state_and_manual_resolution,
        test_runtime_enemy_and_zero_health_target_nonmutation,
        test_player_target_bridge_composition,
        test_decision_and_selection_composition
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("enemy attack execution tests passed");
    return EXIT_SUCCESS;
}
