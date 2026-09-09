#include "enemy_attack_cadence_store.h"
#include "enemy_pursuit_runtime.h"
#include "enemy_target_selection.h"
#include "player_death.h"
#include "player_runtime_population.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,       \
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
    HTHEnemyAttackCadenceStore *cadences;
    HTHPlayerLifecycleRuntime lifecycle;
} Fixture;

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
    fixture->cadences = hth_enemy_attack_cadence_store_create();
    hth_player_lifecycle_runtime_reset(&fixture->lifecycle);
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->enemies != NULL && fixture->spatial != NULL &&
           fixture->bodies != NULL && fixture->health != NULL &&
           fixture->targets != NULL && fixture->cadences != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_enemy_attack_cadence_store_destroy(fixture->cadences);
    hth_enemy_target_store_destroy(fixture->targets);
    hth_health_store_destroy(fixture->health);
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_spatial_store_destroy(fixture->spatial);
    hth_enemy_store_destroy(fixture->enemies);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static HTHPlayerRuntimeSpawnSpec player_spec(float x)
{
    HTHPlayerRuntimeSpawnSpec spec;

    spec.transform.position = hth_vec3(x, 0.0F, 0.0F);
    spec.transform.yaw = 0.0F;
    spec.health = (HTHHealth){100.0F, 100.0F};
    return spec;
}

static bool spawn_players(Fixture *fixture,
                          HTHEntityHandle players[HTH_MAX_PLAYERS])
{
    static const float positions[HTH_MAX_PLAYERS] = {2.0F, 1.0F, 3.0F,
                                                      4.0F};
    size_t index;

    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        HTHPlayerSlot slot;
        HTHPlayerRuntimeSpawnSpec spec = player_spec(positions[index]);

        if (!hth_player_runtime_spawn(
                fixture->entities, fixture->actors, fixture->spatial,
                fixture->bodies, fixture->health, &fixture->lifecycle,
                &spec, &slot) ||
            slot != index ||
            !hth_player_roster_get_slot(
                hth_player_lifecycle_runtime_get_roster(&fixture->lifecycle),
                fixture->entities, fixture->actors, slot, &players[index])) {
            return false;
        }
    }
    return true;
}

static bool create_enemy(Fixture *fixture, HTHEntityHandle *out_enemy)
{
    const HTHSpatialTransform transform = {{0.0F, 0.0F, 0.0F}, 0.0F};
    const HTHDynamicBody body = {{0.5F, 0.5F, 0.5F},
                                 {0.0F, 0.0F, 0.0F}};

    return hth_entity_registry_create_entity(fixture->entities, out_enemy) &&
           hth_actor_store_attach(fixture->actors, fixture->entities,
                                  *out_enemy) &&
           hth_enemy_store_attach(fixture->enemies, fixture->entities,
                                  fixture->actors, *out_enemy) &&
           hth_spatial_store_attach(fixture->spatial, fixture->entities,
                                    *out_enemy, &transform) &&
           hth_dynamic_body_attach(fixture->bodies, fixture->entities,
                                   fixture->spatial, *out_enemy, &body) &&
           hth_enemy_attack_cadence_store_attach(
               fixture->cadences, fixture->entities, fixture->actors,
               fixture->enemies, *out_enemy);
}

static bool build_exclusions(
    const Fixture *fixture,
    const HTHEntityHandle players[HTH_MAX_PLAYERS],
    HTHEntityHandle exclusions[HTH_MAX_PLAYERS],
    size_t *out_exclusion_count)
{
    size_t count = 0U;
    size_t index;

    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        bool dead;

        if (!hth_player_death_is_dead(
                fixture->entities, fixture->actors, fixture->health,
                players[index], &dead)) {
            return false;
        }
        if (dead) {
            exclusions[count++] = players[index];
        }
    }
    *out_exclusion_count = count;
    return true;
}

static bool damage_player(Fixture *fixture, HTHEntityHandle player)
{
    HTHDamageResult result;

    return hth_health_store_apply_damage(
        fixture->health, fixture->entities, fixture->actors, player,
        100.0F, &result);
}

static bool select_target(
    Fixture *fixture, const HTHCollisionWorld *world, HTHEntityHandle enemy,
    const HTHEntityHandle players[HTH_MAX_PLAYERS],
    const HTHEntityHandle exclusions[HTH_MAX_PLAYERS],
    size_t exclusion_count, HTHEntityHandle *out_selected)
{
    return hth_enemy_target_select(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->spatial, world, fixture->targets, enemy, players,
        HTH_MAX_PLAYERS, exclusions, exclusion_count, 10.0F, out_selected);
}

static bool test_four_player_dead_target_exclusion(void)
{
    Fixture fixture;
    HTHCollisionWorld world = {0};
    HTHEntityHandle players[HTH_MAX_PLAYERS];
    HTHEntityHandle exclusions[HTH_MAX_PLAYERS];
    HTHEntityHandle enemy;
    HTHEntityHandle selected;
    HTHEntityHandle non_player;
    HTHSpatialTransform non_player_transform = {{5.0F, 0.0F, 0.0F}, 0.0F};
    HTHHealingResult healing;
    size_t exclusion_count;

    world.obstacles[0] = (HTHAABB){{100.0F, 100.0F, 100.0F},
                                   {101.0F, 101.0F, 101.0F}};
    world.obstacle_count = 1U;
    CHECK(fixture_create(&fixture));
    CHECK(spawn_players(&fixture, players));
    CHECK(create_enemy(&fixture, &enemy));

    CHECK(damage_player(&fixture, players[1]));
    CHECK(damage_player(&fixture, players[3]));
    CHECK(build_exclusions(&fixture, players, exclusions,
                           &exclusion_count));
    CHECK(exclusion_count == 2U);
    CHECK(hth_entity_handle_equal(exclusions[0], players[1]));
    CHECK(hth_entity_handle_equal(exclusions[1], players[3]));

    CHECK(hth_player_defeat_mark(&fixture.lifecycle.defeat[0]));
    CHECK(select_target(&fixture, &world, enemy, players, exclusions,
                        exclusion_count, &selected));
    CHECK(hth_entity_handle_equal(selected, players[0]));

    CHECK(hth_enemy_target_store_set(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, players[1]));
    CHECK(hth_enemy_pursuit_runtime_step(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.bodies, fixture.health, fixture.targets, fixture.cadences,
        &world, players, HTH_MAX_PLAYERS, exclusions, exclusion_count,
        10.0F, 10.0F, 0.0F, 0.0F, 1.0, 0.0));
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &selected));
    CHECK(hth_entity_handle_equal(selected, players[0]));

    CHECK(hth_health_store_apply_healing(
        fixture.health, fixture.entities, fixture.actors, players[1],
        100.0F, &healing));
    CHECK(build_exclusions(&fixture, players, exclusions,
                           &exclusion_count));
    CHECK(exclusion_count == 1U);
    CHECK(hth_enemy_target_store_clear(fixture.targets, fixture.entities,
                                       enemy));
    CHECK(select_target(&fixture, &world, enemy, players, exclusions,
                        exclusion_count, &selected));
    CHECK(hth_entity_handle_equal(selected, players[1]));

    CHECK(damage_player(&fixture, players[0]));
    CHECK(damage_player(&fixture, players[1]));
    CHECK(damage_player(&fixture, players[2]));
    CHECK(build_exclusions(&fixture, players, exclusions,
                           &exclusion_count));
    CHECK(exclusion_count == HTH_MAX_PLAYERS);
    CHECK(hth_enemy_target_store_set(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, players[1]));
    CHECK(hth_enemy_pursuit_runtime_step(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.bodies, fixture.health, fixture.targets, fixture.cadences,
        &world, players, HTH_MAX_PLAYERS, exclusions, exclusion_count,
        10.0F, 10.0F, 0.0F, 0.0F, 1.0, 0.0));
    CHECK(!hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &selected));

    CHECK(hth_entity_registry_create_entity(fixture.entities, &non_player));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   non_player, &non_player_transform));
    CHECK(hth_enemy_target_store_set(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, non_player));
    CHECK(hth_enemy_pursuit_runtime_step(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.bodies, fixture.health, fixture.targets, fixture.cadences,
        &world, players, HTH_MAX_PLAYERS, exclusions, exclusion_count,
        10.0F, 0.0F, 0.0F, 0.0F, 1.0, 0.0));
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &selected));
    CHECK(hth_entity_handle_equal(selected, non_player));

    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    if (!test_four_player_dead_target_exclusion()) {
        return EXIT_FAILURE;
    }
    puts("multi-player dead target exclusion tests passed");
    return EXIT_SUCCESS;
}
