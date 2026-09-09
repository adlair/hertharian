#include "player_revive_target_selection.h"

#include "actor_spawn.h"
#include "player_death.h"
#include "player_runtime_population.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    HTHSpatialStore *spatial;
    HTHDynamicBodyStore *bodies;
    HTHHealthStore *health;
    HTHPlayerLifecycleRuntime lifecycle;
} Fixture;

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->spatial = hth_spatial_store_create();
    fixture->bodies = hth_dynamic_body_store_create();
    fixture->health = hth_health_store_create();
    hth_player_lifecycle_runtime_reset(&fixture->lifecycle);
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->spatial != NULL && fixture->bodies != NULL &&
           fixture->health != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_health_store_destroy(fixture->health);
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_spatial_store_destroy(fixture->spatial);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static HTHPlayerRuntimeSpawnSpec player_spec(float x, float y, float z)
{
    HTHPlayerRuntimeSpawnSpec spec;

    spec.transform.position = hth_vec3(x, y, z);
    spec.transform.yaw = 0.0F;
    spec.health = (HTHHealth){100.0F, 100.0F};
    return spec;
}

static bool spawn_player(Fixture *fixture, float x, float y, float z,
                         HTHPlayerSlot *out_slot,
                         HTHEntityHandle *out_player)
{
    HTHPlayerRuntimeSpawnSpec spec = player_spec(x, y, z);

    return hth_player_runtime_spawn(
               fixture->entities, fixture->actors, fixture->spatial,
               fixture->bodies, fixture->health, &fixture->lifecycle,
               &spec, out_slot) &&
           hth_player_roster_get_slot(
               hth_player_lifecycle_runtime_get_roster(&fixture->lifecycle),
               fixture->entities, fixture->actors, *out_slot, out_player);
}

static bool spawn_unregistered_player(Fixture *fixture, float x, float y,
                                      float z,
                                      HTHEntityHandle *out_player)
{
    HTHActorSpawnSpec spec = {0};

    spec.has_spatial = true;
    spec.transform = player_spec(x, y, z).transform;
    spec.has_health = true;
    spec.health = (HTHHealth){100.0F, 100.0F};
    return hth_actor_spawn(fixture->entities, fixture->actors,
                           fixture->spatial, fixture->bodies,
                           fixture->health, &spec, out_player);
}

static bool spawn_four(Fixture *fixture,
                       HTHPlayerSlot slots[HTH_MAX_PLAYERS],
                       HTHEntityHandle players[HTH_MAX_PLAYERS])
{
    static const float positions[HTH_MAX_PLAYERS] = {0.0F, 6.0F, 3.0F,
                                                      2.0F};
    size_t index;

    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        if (!spawn_player(fixture, positions[index], 0.0F, 0.0F,
                          &slots[index], &players[index]) ||
            slots[index] != index) {
            return false;
        }
    }
    return true;
}

static bool select_target(Fixture *fixture, HTHPlayerSlot reviver,
                          float range, HTHPlayerSlot *out_target)
{
    return hth_player_revive_target_select(
        &fixture->lifecycle, fixture->entities, fixture->actors,
        fixture->health, fixture->spatial, reviver, range, out_target);
}

static bool make_dead(Fixture *fixture, HTHEntityHandle player)
{
    HTHDamageResult result;

    return hth_health_store_apply_damage(
        fixture->health, fixture->entities, fixture->actors, player,
        100.0F, &result);
}

static bool begin_downed(Fixture *fixture, HTHPlayerSlot slot,
                         HTHEntityHandle player, double duration)
{
    bool dead;

    return make_dead(fixture, player) &&
           hth_player_death_is_dead(
               fixture->entities, fixture->actors, fixture->health,
               player, &dead) &&
           dead &&
           hth_player_lifecycle_runtime_step_player(
               &fixture->lifecycle, fixture->entities, fixture->actors,
               slot, dead, 0.0, &duration);
}

static bool set_position(Fixture *fixture, HTHEntityHandle player,
                         float x, float y, float z)
{
    HTHSpatialTransform transform;

    if (!hth_spatial_store_get(fixture->spatial, fixture->entities,
                               player, &transform)) {
        return false;
    }
    transform.position = hth_vec3(x, y, z);
    return hth_spatial_store_set(fixture->spatial, fixture->entities,
                                 player, &transform);
}

static bool test_validation_and_canonical_output(void)
{
    Fixture fixture;
    HTHPlayerSlot slot;
    HTHEntityHandle player;
    HTHPlayerSlot target;
    const float invalid_ranges[] = {0.0F, -1.0F, NAN, INFINITY, -INFINITY};
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_player(&fixture, 0.0F, 0.0F, 0.0F, &slot, &player));

#define CHECK_FAILURE(call)                                                  \
    do {                                                                     \
        target = slot;                                                       \
        CHECK(!(call));                                                      \
        CHECK(target == HTH_PLAYER_SLOT_INVALID);                            \
    } while (0)

    CHECK_FAILURE(hth_player_revive_target_select(
        NULL, fixture.entities, fixture.actors, fixture.health,
        fixture.spatial, slot, 1.0F, &target));
    CHECK_FAILURE(hth_player_revive_target_select(
        &fixture.lifecycle, NULL, fixture.actors, fixture.health,
        fixture.spatial, slot, 1.0F, &target));
    CHECK_FAILURE(hth_player_revive_target_select(
        &fixture.lifecycle, fixture.entities, NULL, fixture.health,
        fixture.spatial, slot, 1.0F, &target));
    CHECK_FAILURE(hth_player_revive_target_select(
        &fixture.lifecycle, fixture.entities, fixture.actors, NULL,
        fixture.spatial, slot, 1.0F, &target));
    CHECK_FAILURE(hth_player_revive_target_select(
        &fixture.lifecycle, fixture.entities, fixture.actors,
        fixture.health, NULL, slot, 1.0F, &target));
    CHECK(!hth_player_revive_target_select(
        &fixture.lifecycle, fixture.entities, fixture.actors,
        fixture.health, fixture.spatial, slot, 1.0F, NULL));
    for (index = 0U; index < sizeof(invalid_ranges) /
                               sizeof(invalid_ranges[0]); ++index) {
        CHECK_FAILURE(select_target(&fixture, slot, invalid_ranges[index],
                                    &target));
    }
    CHECK_FAILURE(select_target(&fixture, HTH_PLAYER_SLOT_INVALID, 1.0F,
                                &target));
    CHECK_FAILURE(select_target(&fixture, HTH_MAX_PLAYERS + 1U, 1.0F,
                                &target));
    CHECK_FAILURE(select_target(&fixture, 1U, 1.0F, &target));
#undef CHECK_FAILURE

    CHECK(select_target(&fixture, slot, 1.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);
    fixture_destroy(&fixture);
    return true;
}

static bool test_reviver_component_and_generation_failures(void)
{
    Fixture fixture;
    HTHPlayerSlot slot;
    HTHPlayerSlot target;
    HTHEntityHandle player;
    HTHHealth health;
    HTHSpatialTransform transform;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_player(&fixture, 0.0F, 0.0F, 0.0F, &slot, &player));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, player, &health));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, player));
    CHECK(!select_target(&fixture, slot, 1.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, player, health));

    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, player,
                                &transform));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   player));
    CHECK(!select_target(&fixture, slot, 1.0F, &target));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities, player,
                                   &transform));

    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, player));
    CHECK(!select_target(&fixture, slot, 1.0F, &target));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, player));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, player));
    CHECK(!select_target(&fixture, slot, 1.0F, &target));
    fixture_destroy(&fixture);
    return true;
}

static bool test_policy_filter_and_spatial_order(void)
{
    Fixture fixture;
    HTHPlayerSlot slots[HTH_MAX_PLAYERS];
    HTHEntityHandle players[HTH_MAX_PLAYERS];
    HTHPlayerSlot target;
    HTHHealingResult healing;
    double duration = 10.0;
    bool dead;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_four(&fixture, slots, players));
    CHECK(make_dead(&fixture, players[2]));
    CHECK(begin_downed(&fixture, slots[3], players[3], duration));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   players[1]));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   players[2]));
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == slots[3]);

    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   players[3]));
    CHECK(hth_player_defeat_mark(&fixture.lifecycle.defeat[slots[3]]));
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);
    hth_player_defeat_reset(&fixture.lifecycle.defeat[slots[3]]);
    CHECK(!select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);

    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, players[3]));
    CHECK(!select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);

    CHECK(hth_health_store_apply_healing(
        fixture.health, fixture.entities, fixture.actors, players[2],
        100.0F, &healing));
    CHECK(hth_player_death_is_dead(
        fixture.entities, fixture.actors, fixture.health, players[2],
        &dead));
    CHECK(!dead);
    CHECK(hth_player_lifecycle_runtime_step_player(
        &fixture.lifecycle, fixture.entities, fixture.actors, slots[2],
        dead, 0.0, &duration));
    fixture_destroy(&fixture);
    return true;
}

static bool test_reviver_policy_states(void)
{
    Fixture fixture;
    HTHPlayerSlot slots[HTH_MAX_PLAYERS];
    HTHEntityHandle players[HTH_MAX_PLAYERS];
    HTHPlayerSlot target;
    HTHHealingResult healing;
    double duration = 1.0;
    bool dead;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_four(&fixture, slots, players));
    CHECK(begin_downed(&fixture, slots[2], players[2], 10.0));
    CHECK(begin_downed(&fixture, slots[0], players[0], duration));
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);

    CHECK(hth_player_lifecycle_runtime_step_player(
        &fixture.lifecycle, fixture.entities, fixture.actors, slots[0],
        true, duration, &duration));
    CHECK(hth_health_store_apply_healing(
        fixture.health, fixture.entities, fixture.actors, players[0],
        100.0F, &healing));
    CHECK(hth_player_death_is_dead(
        fixture.entities, fixture.actors, fixture.health, players[0],
        &dead));
    CHECK(!dead);
    CHECK(hth_player_lifecycle_runtime_step_player(
        &fixture.lifecycle, fixture.entities, fixture.actors, slots[0],
        dead, 0.0, &duration));
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);
    fixture_destroy(&fixture);
    return true;
}

static bool test_ranking_range_tie_switch_and_determinism(void)
{
    Fixture fixture;
    HTHPlayerSlot slots[HTH_MAX_PLAYERS];
    HTHEntityHandle players[HTH_MAX_PLAYERS];
    HTHPlayerSlot target;
    HTHPlayerLifecycleRuntime lifecycle_before;
    HTHHealth health_before;
    HTHHealth health_after;
    HTHSpatialTransform spatial_before;
    HTHSpatialTransform spatial_after;
    size_t iteration;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_four(&fixture, slots, players));
    CHECK(begin_downed(&fixture, slots[2], players[2], 10.0));
    CHECK(begin_downed(&fixture, slots[3], players[3], 10.0));
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == slots[3]);

    CHECK(set_position(&fixture, players[2], 1.0F, 0.0F, 0.0F));
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == slots[2]);
    CHECK(set_position(&fixture, players[2], 2.0F, 0.0F, 0.0F));
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == slots[2]);

    CHECK(select_target(&fixture, slots[0], 2.0F, &target));
    CHECK(target == slots[2]);
    CHECK(select_target(&fixture, slots[0], 1.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);
    CHECK(set_position(&fixture, players[2], 0.0F, 0.0F, 0.0F));
    CHECK(select_target(&fixture, slots[0], FLT_MIN, &target));
    CHECK(target == slots[2]);

    CHECK(select_target(&fixture, slots[1], 10.0F, &target));
    CHECK(target == slots[3]);

    lifecycle_before = fixture.lifecycle;
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, players[2], &health_before));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                players[2], &spatial_before));
    for (iteration = 0U; iteration < 128U; ++iteration) {
        CHECK(select_target(&fixture, slots[0], 10.0F, &target));
        CHECK(target == slots[2]);
    }
    for (iteration = 0U; iteration < HTH_MAX_PLAYERS; ++iteration) {
        HTHEntityHandle current;

        CHECK(hth_player_roster_get_slot(
            hth_player_lifecycle_runtime_get_roster(&fixture.lifecycle),
            fixture.entities, fixture.actors, slots[iteration], &current));
        CHECK(hth_entity_handle_equal(current, players[iteration]));
        CHECK(hth_entity_registry_is_alive(fixture.entities, current));
        CHECK(hth_actor_store_has(fixture.actors, fixture.entities,
                                  current));
    }
    CHECK(memcmp(&lifecycle_before, &fixture.lifecycle,
                 sizeof(lifecycle_before)) == 0);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, players[2], &health_after));
    CHECK(memcmp(&health_before, &health_after, sizeof(health_before)) == 0);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                players[2], &spatial_after));
    CHECK(memcmp(&spatial_before, &spatial_after,
                 sizeof(spatial_before)) == 0);
    fixture_destroy(&fixture);
    return true;
}

static bool test_tie_uses_player_slot_not_entity_index(void)
{
    Fixture fixture;
    HTHEntityHandle reviver;
    HTHEntityHandle lower_entity_index;
    HTHEntityHandle higher_entity_index;
    HTHPlayerSlot reviver_slot;
    HTHPlayerSlot higher_index_slot;
    HTHPlayerSlot lower_index_slot;
    HTHPlayerSlot target;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_unregistered_player(&fixture, 0.0F, 0.0F, 0.0F,
                                    &reviver));
    CHECK(spawn_unregistered_player(&fixture, 1.0F, 0.0F, 0.0F,
                                    &lower_entity_index));
    CHECK(spawn_unregistered_player(&fixture, -1.0F, 0.0F, 0.0F,
                                    &higher_entity_index));
    CHECK(lower_entity_index.index < higher_entity_index.index);
    CHECK(hth_player_lifecycle_runtime_register(
        &fixture.lifecycle, fixture.entities, fixture.actors, reviver,
        &reviver_slot));
    CHECK(hth_player_lifecycle_runtime_register(
        &fixture.lifecycle, fixture.entities, fixture.actors,
        higher_entity_index, &higher_index_slot));
    CHECK(hth_player_lifecycle_runtime_register(
        &fixture.lifecycle, fixture.entities, fixture.actors,
        lower_entity_index, &lower_index_slot));
    CHECK(reviver_slot == 0U);
    CHECK(higher_index_slot < lower_index_slot);
    CHECK(begin_downed(&fixture, higher_index_slot, higher_entity_index,
                       10.0));
    CHECK(begin_downed(&fixture, lower_index_slot, lower_entity_index,
                       10.0));
    CHECK(select_target(&fixture, reviver_slot, 10.0F, &target));
    CHECK(target == higher_index_slot);
    fixture_destroy(&fixture);
    return true;
}

static bool test_sparse_stale_malformed_and_reuse(void)
{
    Fixture fixture;
    HTHPlayerSlot slots[HTH_MAX_PLAYERS];
    HTHEntityHandle players[HTH_MAX_PLAYERS];
    HTHPlayerSlot replacement_slot;
    HTHEntityHandle replacement;
    HTHPlayerSlot target;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_four(&fixture, slots, players));
    CHECK(begin_downed(&fixture, slots[2], players[2], 10.0));
    CHECK(hth_player_runtime_despawn(
        fixture.entities, fixture.actors, fixture.spatial, fixture.bodies,
        fixture.health, &fixture.lifecycle, slots[1]));
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == slots[2]);

    CHECK(spawn_player(&fixture, 1.0F, 0.0F, 0.0F, &replacement_slot,
                       &replacement));
    CHECK(replacement_slot == slots[1]);

    CHECK(hth_player_runtime_despawn(
        fixture.entities, fixture.actors, fixture.spatial, fixture.bodies,
        fixture.health, &fixture.lifecycle, slots[2]));
    CHECK(spawn_player(&fixture, 1.0F, 0.0F, 0.0F, &replacement_slot,
                       &replacement));
    CHECK(replacement_slot == slots[2]);
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);
    CHECK(begin_downed(&fixture, replacement_slot, replacement, 10.0));
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == slots[2]);

    fixture.lifecycle.revive_windows[slots[3]].active = true;
    fixture.lifecycle.revive_windows[slots[3]].remaining_seconds = 0.0;
    CHECK(!select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);
    fixture_destroy(&fixture);

    CHECK(fixture_create(&fixture));
    CHECK(spawn_four(&fixture, slots, players));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, players[3]));
    CHECK(!select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);
    fixture_destroy(&fixture);
    return true;
}

static bool test_lifecycle_changes_and_large_values(void)
{
    Fixture fixture;
    HTHPlayerSlot slots[HTH_MAX_PLAYERS];
    HTHEntityHandle players[HTH_MAX_PLAYERS];
    HTHPlayerSlot target;
    bool revived;
    bool expired;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_four(&fixture, slots, players));
    CHECK(begin_downed(&fixture, slots[2], players[2], 10.0));
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == slots[2]);
    CHECK(hth_player_lifecycle_runtime_execute_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors,
        fixture.health, slots[1], slots[2], 100.0F, &revived));
    CHECK(revived);
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);

    CHECK(begin_downed(&fixture, slots[3], players[3], 1.0));
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == slots[3]);
    CHECK(hth_player_defeat_mark(&fixture.lifecycle.defeat[slots[3]]));
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);
    hth_player_defeat_reset(&fixture.lifecycle.defeat[slots[3]]);
    CHECK(hth_player_revive_window_advance(
        &fixture.lifecycle.revive_windows[slots[3]], 1.0, &expired));
    CHECK(expired);
    CHECK(select_target(&fixture, slots[0], 10.0F, &target));
    CHECK(target == HTH_PLAYER_SLOT_INVALID);
    fixture_destroy(&fixture);

    CHECK(fixture_create(&fixture));
    CHECK(spawn_player(&fixture, FLT_MAX * 0.75F, FLT_MAX * 0.75F,
                       FLT_MAX * 0.75F, &slots[0], &players[0]));
    CHECK(spawn_player(&fixture, -FLT_MAX * 0.10F, FLT_MAX * 0.75F,
                       FLT_MAX * 0.75F, &slots[1], &players[1]));
    CHECK(begin_downed(&fixture, slots[1], players[1], 10.0));
    CHECK(select_target(&fixture, slots[0], FLT_MAX, &target));
    CHECK(target == slots[1]);
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    static bool (*const tests[])(void) = {
        test_validation_and_canonical_output,
        test_reviver_component_and_generation_failures,
        test_policy_filter_and_spatial_order,
        test_reviver_policy_states,
        test_ranking_range_tie_switch_and_determinism,
        test_tie_uses_player_slot_not_entity_index,
        test_sparse_stale_malformed_and_reuse,
        test_lifecycle_changes_and_large_values
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("player revive target selection tests passed");
    return EXIT_SUCCESS;
}
