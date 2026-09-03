#include "enemy_attack_cadence.h"

#include "damage_intent.h"
#include "dynamic_body.h"
#include "enemy.h"
#include "enemy_attack_execution.h"
#include "enemy_decision.h"
#include "enemy_target.h"
#include "entity.h"
#include "health.h"
#include "player_target_bridge.h"
#include "spatial.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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
    HTHSpatialStore *spatial;
    HTHDynamicBodyStore *bodies;
    HTHHealthStore *health;
    HTHEnemyTargetStore *targets;
} Fixture;

typedef struct {
    double sixty;
    double thirty;
    double one;
    double quarters;
    double eighths;
    double mixed;
    bool sixty_ready;
    bool thirty_ready;
    bool one_ready;
    bool quarters_ready;
    bool eighths_ready;
    bool mixed_ready;
} PartitionResults;

static PartitionResults partition_results;

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->enemies = hth_enemy_store_create();
    fixture->spatial = hth_spatial_store_create();
    fixture->bodies = hth_dynamic_body_store_create();
    fixture->health = hth_health_store_create();
    fixture->targets = hth_enemy_target_store_create();
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->enemies != NULL && fixture->spatial != NULL &&
           fixture->bodies != NULL && fixture->health != NULL &&
           fixture->targets != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_enemy_target_store_destroy(fixture->targets);
    hth_health_store_destroy(fixture->health);
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_spatial_store_destroy(fixture->spatial);
    hth_enemy_store_destroy(fixture->enemies);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static bool create_actor(Fixture *fixture, HTHEntityHandle *out_actor)
{
    return hth_entity_registry_create_entity(fixture->entities, out_actor) &&
           hth_actor_store_attach(fixture->actors, fixture->entities,
                                  *out_actor);
}

static bool create_enemy(Fixture *fixture, HTHEntityHandle *out_enemy)
{
    return create_actor(fixture, out_enemy) &&
           hth_enemy_store_attach(fixture->enemies, fixture->entities,
                                  fixture->actors, *out_enemy);
}

static bool cadence_ready(const HTHEnemyAttackCadence *cadence)
{
    bool ready = false;

    return hth_enemy_attack_cadence_is_ready(cadence, &ready) && ready;
}

static bool cadence_not_ready(const HTHEnemyAttackCadence *cadence)
{
    bool ready = true;

    return hth_enemy_attack_cadence_is_ready(cadence, &ready) && !ready;
}

static bool test_reset_and_readiness(void)
{
    HTHEnemyAttackCadence cadence = {0.0};
    bool ready;

    hth_enemy_attack_cadence_reset(NULL);
    CHECK(sizeof(cadence) == sizeof(double));
    CHECK(cadence_ready(&cadence));

    cadence.remaining_seconds = 2.0;
    hth_enemy_attack_cadence_reset(&cadence);
    CHECK(cadence.remaining_seconds == 0.0 && cadence_ready(&cadence));
    cadence.remaining_seconds = -1.0;
    hth_enemy_attack_cadence_reset(&cadence);
    CHECK(cadence.remaining_seconds == 0.0);
    cadence.remaining_seconds = NAN;
    hth_enemy_attack_cadence_reset(&cadence);
    CHECK(cadence.remaining_seconds == 0.0);
    cadence.remaining_seconds = INFINITY;
    hth_enemy_attack_cadence_reset(&cadence);
    CHECK(cadence.remaining_seconds == 0.0);
    cadence.remaining_seconds = -INFINITY;
    hth_enemy_attack_cadence_reset(&cadence);
    CHECK(cadence.remaining_seconds == 0.0);

    ready = true;
    CHECK(!hth_enemy_attack_cadence_is_ready(NULL, &ready));
    CHECK(!ready);
    CHECK(!hth_enemy_attack_cadence_is_ready(&cadence, NULL));
    cadence.remaining_seconds = 0.0;
    CHECK(hth_enemy_attack_cadence_is_ready(&cadence, &ready) && ready);
    CHECK(cadence.remaining_seconds == 0.0);
    cadence.remaining_seconds = 0.5;
    CHECK(hth_enemy_attack_cadence_is_ready(&cadence, &ready) && !ready);
    CHECK(cadence.remaining_seconds == 0.5);

    cadence.remaining_seconds = -1.0;
    ready = true;
    CHECK(!hth_enemy_attack_cadence_is_ready(&cadence, &ready) && !ready);
    CHECK(cadence.remaining_seconds == -1.0);
    cadence.remaining_seconds = NAN;
    ready = true;
    CHECK(!hth_enemy_attack_cadence_is_ready(&cadence, &ready) && !ready);
    CHECK(isnan(cadence.remaining_seconds));
    cadence.remaining_seconds = INFINITY;
    ready = true;
    CHECK(!hth_enemy_attack_cadence_is_ready(&cadence, &ready) && !ready);
    CHECK(isinf(cadence.remaining_seconds) && cadence.remaining_seconds > 0.0);
    cadence.remaining_seconds = -INFINITY;
    ready = true;
    CHECK(!hth_enemy_attack_cadence_is_ready(&cadence, &ready) && !ready);
    CHECK(isinf(cadence.remaining_seconds) && cadence.remaining_seconds < 0.0);
    return true;
}

static bool test_advance_contract(void)
{
    HTHEnemyAttackCadence cadence = {1.0};

    CHECK(!hth_enemy_attack_cadence_advance(NULL, 0.25));
    CHECK(!hth_enemy_attack_cadence_advance(&cadence, -0.25));
    CHECK(cadence.remaining_seconds == 1.0);
    CHECK(!hth_enemy_attack_cadence_advance(&cadence, NAN));
    CHECK(cadence.remaining_seconds == 1.0);
    CHECK(!hth_enemy_attack_cadence_advance(&cadence, INFINITY));
    CHECK(cadence.remaining_seconds == 1.0);
    CHECK(!hth_enemy_attack_cadence_advance(&cadence, -INFINITY));
    CHECK(cadence.remaining_seconds == 1.0);
    CHECK(hth_enemy_attack_cadence_advance(&cadence, 0.0));
    CHECK(cadence.remaining_seconds == 1.0);
    CHECK(hth_enemy_attack_cadence_advance(&cadence, 0.25));
    CHECK(cadence.remaining_seconds == 0.75);
    CHECK(hth_enemy_attack_cadence_advance(&cadence, 0.75));
    CHECK(cadence.remaining_seconds == 0.0 && cadence_ready(&cadence));
    CHECK(hth_enemy_attack_cadence_advance(&cadence, 0.0));
    CHECK(cadence.remaining_seconds == 0.0);

    cadence.remaining_seconds = 1.0;
    CHECK(hth_enemy_attack_cadence_advance(&cadence, 2.0));
    CHECK(cadence.remaining_seconds == 0.0);
    cadence.remaining_seconds = -1.0;
    CHECK(!hth_enemy_attack_cadence_advance(&cadence, 0.25));
    CHECK(cadence.remaining_seconds == -1.0);
    cadence.remaining_seconds = NAN;
    CHECK(!hth_enemy_attack_cadence_advance(&cadence, 0.25));
    CHECK(isnan(cadence.remaining_seconds));
    cadence.remaining_seconds = INFINITY;
    CHECK(!hth_enemy_attack_cadence_advance(&cadence, 0.25));
    CHECK(isinf(cadence.remaining_seconds) && cadence.remaining_seconds > 0.0);
    cadence.remaining_seconds = -INFINITY;
    CHECK(!hth_enemy_attack_cadence_advance(&cadence, 0.25));
    CHECK(isinf(cadence.remaining_seconds) && cadence.remaining_seconds < 0.0);
    return true;
}

static bool test_commit_contract(void)
{
    HTHEnemyAttackCadence cadence = {0.0};

    CHECK(!hth_enemy_attack_cadence_commit(NULL, 1.0));
    CHECK(!hth_enemy_attack_cadence_commit(&cadence, -1.0));
    CHECK(cadence.remaining_seconds == 0.0);
    CHECK(!hth_enemy_attack_cadence_commit(&cadence, NAN));
    CHECK(cadence.remaining_seconds == 0.0);
    CHECK(!hth_enemy_attack_cadence_commit(&cadence, INFINITY));
    CHECK(cadence.remaining_seconds == 0.0);
    CHECK(!hth_enemy_attack_cadence_commit(&cadence, -INFINITY));
    CHECK(cadence.remaining_seconds == 0.0);
    CHECK(hth_enemy_attack_cadence_commit(&cadence, 0.0));
    CHECK(cadence.remaining_seconds == 0.0 && cadence_ready(&cadence));
    CHECK(hth_enemy_attack_cadence_commit(&cadence, 1.5));
    CHECK(cadence.remaining_seconds == 1.5 && cadence_not_ready(&cadence));
    CHECK(!hth_enemy_attack_cadence_commit(&cadence, 2.0));
    CHECK(cadence.remaining_seconds == 1.5);
    CHECK(!hth_enemy_attack_cadence_commit(&cadence, 0.0));
    CHECK(cadence.remaining_seconds == 1.5);

    cadence.remaining_seconds = -1.0;
    CHECK(!hth_enemy_attack_cadence_commit(&cadence, 1.0));
    CHECK(cadence.remaining_seconds == -1.0);
    cadence.remaining_seconds = NAN;
    CHECK(!hth_enemy_attack_cadence_commit(&cadence, 1.0));
    CHECK(isnan(cadence.remaining_seconds));
    cadence.remaining_seconds = INFINITY;
    CHECK(!hth_enemy_attack_cadence_commit(&cadence, 1.0));
    CHECK(isinf(cadence.remaining_seconds) && cadence.remaining_seconds > 0.0);
    cadence.remaining_seconds = -INFINITY;
    CHECK(!hth_enemy_attack_cadence_commit(&cadence, 1.0));
    CHECK(isinf(cadence.remaining_seconds) && cadence.remaining_seconds < 0.0);
    return true;
}

static bool run_partition(const double *deltas, size_t count,
                          double *out_remaining, bool *out_ready)
{
    HTHEnemyAttackCadence cadence = {0.0};
    size_t index;

    CHECK(hth_enemy_attack_cadence_commit(&cadence, 1.0));
    for (index = 0U; index < count; ++index) {
        CHECK(hth_enemy_attack_cadence_advance(&cadence, deltas[index]));
    }
    CHECK(hth_enemy_attack_cadence_is_ready(&cadence, out_ready));
    *out_remaining = cadence.remaining_seconds;
    return true;
}

static bool test_partitions_no_catchup_and_determinism(void)
{
    double sixty[60];
    double thirty[30];
    const double one[] = {1.0};
    const double quarters[] = {0.25, 0.25, 0.25, 0.25};
    const double eighths[] = {
        0.125, 0.125, 0.125, 0.125,
        0.125, 0.125, 0.125, 0.125
    };
    const double mixed[] = {0.125, 0.25, 0.5, 0.125};
    HTHEnemyAttackCadence cadence = {0.0};
    HTHEnemyAttackCadence first = {0.0};
    HTHEnemyAttackCadence second = {0.0};
    HTHEnemyAttackCadence decimal_remainder = {0.0};
    size_t index;

    for (index = 0U; index < 60U; ++index) {
        sixty[index] = 1.0 / 60.0;
    }
    for (index = 0U; index < 30U; ++index) {
        thirty[index] = 1.0 / 30.0;
    }
    CHECK(run_partition(sixty, 60U, &partition_results.sixty,
                        &partition_results.sixty_ready));
    CHECK(run_partition(thirty, 30U, &partition_results.thirty,
                        &partition_results.thirty_ready));
    CHECK(run_partition(one, 1U, &partition_results.one,
                        &partition_results.one_ready));
    CHECK(run_partition(quarters, 4U, &partition_results.quarters,
                        &partition_results.quarters_ready));
    CHECK(run_partition(eighths, 8U, &partition_results.eighths,
                        &partition_results.eighths_ready));
    CHECK(run_partition(mixed, 4U, &partition_results.mixed,
                        &partition_results.mixed_ready));
    CHECK(partition_results.sixty_ready ==
          (partition_results.sixty == 0.0));
    CHECK(partition_results.thirty_ready ==
          (partition_results.thirty == 0.0));
    CHECK(partition_results.one == 0.0 && partition_results.one_ready);
    CHECK(partition_results.quarters == 0.0 &&
          partition_results.quarters_ready);
    CHECK(partition_results.eighths == 0.0 &&
          partition_results.eighths_ready);
    CHECK(partition_results.mixed == 0.0 &&
          partition_results.mixed_ready);

    CHECK(hth_enemy_attack_cadence_commit(&decimal_remainder, 1.0));
    for (index = 0U; index < 30U; ++index) {
        CHECK(hth_enemy_attack_cadence_advance(
            &decimal_remainder, 1.0 / 30.0));
    }
    CHECK(decimal_remainder.remaining_seconds == partition_results.thirty);
    CHECK(cadence_not_ready(&decimal_remainder));
    CHECK(hth_enemy_attack_cadence_advance(
        &decimal_remainder, 1.0 / 30.0));
    CHECK(cadence_ready(&decimal_remainder));

    CHECK(hth_enemy_attack_cadence_commit(&cadence, 1.0));
    CHECK(hth_enemy_attack_cadence_advance(&cadence, 5.0));
    CHECK(cadence_ready(&cadence));
    CHECK(hth_enemy_attack_cadence_advance(&cadence, 5.0));
    CHECK(cadence_ready(&cadence));
    CHECK(hth_enemy_attack_cadence_commit(&cadence, 1.0));
    CHECK(cadence_not_ready(&cadence));

    CHECK(hth_enemy_attack_cadence_commit(&first, 1.0));
    CHECK(hth_enemy_attack_cadence_commit(&second, 2.0));
    CHECK(hth_enemy_attack_cadence_advance(&first, 1.0));
    CHECK(cadence_ready(&first));
    CHECK(second.remaining_seconds == 2.0 && cadence_not_ready(&second));
    CHECK(hth_enemy_attack_cadence_advance(&second, 0.5));
    CHECK(second.remaining_seconds == 1.5);

    for (index = 0U; index < 128U; ++index) {
        hth_enemy_attack_cadence_reset(&cadence);
        CHECK(cadence_ready(&cadence));
        CHECK(hth_enemy_attack_cadence_commit(&cadence, 1.0));
        CHECK(cadence_not_ready(&cadence));
        CHECK(hth_enemy_attack_cadence_advance(&cadence, 0.25));
        CHECK(cadence.remaining_seconds == 0.75);
        CHECK(hth_enemy_attack_cadence_advance(&cadence, 0.75));
        CHECK(cadence_ready(&cadence));
    }
    return true;
}

static bool test_attack_execution_composition(void)
{
    Fixture fixture;
    HTHEnemyAttackCadence cadence = {0.0};
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle no_health_target;
    HTHDamageIntent intent;
    HTHDamageResolution resolution;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &enemy));
    CHECK(create_actor(&fixture, &target));
    CHECK(create_actor(&fixture, &no_health_target));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, target,
                                  (HTHHealth){100.0F, 100.0F}));

    CHECK(cadence_ready(&cadence));
    CHECK(hth_enemy_attack_build_damage_intent(
        fixture.entities, fixture.actors, fixture.enemies, enemy, target,
        10.0F, &intent));
    CHECK(hth_enemy_attack_cadence_commit(&cadence, 1.0));
    CHECK(cadence_not_ready(&cadence));
    CHECK(hth_enemy_attack_cadence_advance(&cadence, 0.25));
    CHECK(cadence_not_ready(&cadence));
    CHECK(hth_enemy_attack_cadence_advance(&cadence, 0.75));
    CHECK(cadence_ready(&cadence));
    CHECK(hth_enemy_attack_build_damage_intent(
        fixture.entities, fixture.actors, fixture.enemies, enemy, target,
        10.0F, &intent));

    hth_enemy_attack_cadence_reset(&cadence);
    CHECK(!hth_enemy_attack_build_damage_intent(
        fixture.entities, fixture.actors, fixture.enemies, enemy,
        hth_entity_handle_invalid(), 10.0F, &intent));
    CHECK(cadence_ready(&cadence));

    CHECK(hth_enemy_attack_build_damage_intent(
        fixture.entities, fixture.actors, fixture.enemies, enemy,
        no_health_target, 10.0F, &intent));
    CHECK(hth_enemy_attack_cadence_commit(&cadence, 1.0));
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(!resolution.applied);
    CHECK(cadence_not_ready(&cadence));

    hth_enemy_attack_cadence_reset(&cadence);
    CHECK(hth_enemy_attack_build_damage_intent(
        fixture.entities, fixture.actors, fixture.enemies, enemy, target,
        10.0F, &intent));
    CHECK(hth_enemy_attack_cadence_commit(&cadence, 0.0));
    CHECK(cadence_ready(&cadence));
    fixture_destroy(&fixture);
    return true;
}

static bool test_decision_attack_composition(void)
{
    Fixture fixture;
    const HTHCollisionWorld clear_world = {0};
    HTHEnemyAttackCadence cadence = {0.0};
    HTHPlayerTargetBridge bridge = {hth_entity_handle_invalid()};
    HTHPlayerBody player;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHSpatialTransform target_transform;
    HTHEnemyIntent decision;
    HTHDamageIntent damage_intent;
    HTHDamageResolution resolution;
    HTHHealth health;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &enemy));
    CHECK(hth_player_body_init(&player, (HTHVec3){0.0F, 0.0F, 0.0F}));
    CHECK(hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, fixture.health, &player,
        (HTHHealth){100.0F, 100.0F}));
    CHECK(hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &target));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &target_transform));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   enemy, &target_transform));
    CHECK(hth_enemy_target_store_set(
        fixture.targets, fixture.entities, fixture.actors,
        fixture.enemies, enemy, target));

    CHECK(hth_enemy_decision_evaluate_with_attack(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &clear_world, enemy, 5.0F, 1.0F, &decision));
    CHECK(decision.kind == HTH_ENEMY_INTENT_ATTACK);
    CHECK(cadence_ready(&cadence));
    CHECK(hth_enemy_attack_build_damage_intent(
        fixture.entities, fixture.actors, fixture.enemies, enemy,
        decision.target, 12.0F, &damage_intent));
    CHECK(hth_enemy_attack_cadence_commit(&cadence, 1.0));
    CHECK(hth_damage_intent_resolve(
        &damage_intent, fixture.entities, fixture.actors, fixture.health,
        &resolution));
    CHECK(resolution.applied && resolution.damage.applied == 12.0F);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &health));
    CHECK(health.current == 88.0F);

    CHECK(hth_enemy_decision_evaluate_with_attack(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &clear_world, enemy, 5.0F, 1.0F, &decision));
    CHECK(decision.kind == HTH_ENEMY_INTENT_ATTACK);
    CHECK(cadence_not_ready(&cadence));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &health));
    CHECK(health.current == 88.0F);
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_reset_and_readiness,
        test_advance_contract,
        test_commit_contract,
        test_partitions_no_catchup_and_determinism,
        test_attack_execution_composition,
        test_decision_attack_composition
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    printf("cadence partition remaining_seconds/readiness:\n"
           "  60x1/60=%.17g/%s\n"
           "  30x1/30=%.17g/%s\n"
           "  1x1=%.17g/%s\n"
           "  4x0.25=%.17g/%s\n"
           "  8x0.125=%.17g/%s\n"
           "  mixed=%.17g/%s\n",
           partition_results.sixty,
           partition_results.sixty_ready ? "ready" : "not-ready",
           partition_results.thirty,
           partition_results.thirty_ready ? "ready" : "not-ready",
           partition_results.one,
           partition_results.one_ready ? "ready" : "not-ready",
           partition_results.quarters,
           partition_results.quarters_ready ? "ready" : "not-ready",
           partition_results.eighths,
           partition_results.eighths_ready ? "ready" : "not-ready",
           partition_results.mixed,
           partition_results.mixed_ready ? "ready" : "not-ready");
    puts("enemy attack cadence tests passed");
    return EXIT_SUCCESS;
}
