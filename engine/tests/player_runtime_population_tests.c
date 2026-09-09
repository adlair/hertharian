#include "player_runtime_population.h"

#include "player_target_bridge.h"

#include <math.h>
#include <stdio.h>

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        fprintf(stderr, "check failed: %s:%d: %s\n",                      \
                __FILE__, __LINE__, #condition);                             \
        return false;                                                        \
    }                                                                        \
} while (0)

typedef struct Fixture {
    HTHEntityRegistry *entities;
    HTHActorStore *actors;
    HTHSpatialStore *spatial;
    HTHDynamicBodyStore *bodies;
    HTHHealthStore *health;
    HTHPlayerLifecycleRuntime lifecycle;
} Fixture;

static bool fixture_init(Fixture *fixture)
{
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

static HTHPlayerRuntimeSpawnSpec player_spec(float seed)
{
    HTHPlayerRuntimeSpawnSpec spec;

    spec.transform.position = hth_vec3(seed, seed + 1.0F, seed + 2.0F);
    spec.transform.yaw = seed * 0.125F;
    spec.health.current = 10.0F + seed;
    spec.health.maximum = 20.0F + seed;
    return spec;
}

static bool spawn_player(Fixture *fixture,
                         HTHPlayerRuntimeSpawnSpec spec,
                         HTHPlayerSlot *out_slot)
{
    return hth_player_runtime_spawn(
        fixture->entities, fixture->actors, fixture->spatial,
        fixture->bodies, fixture->health, &fixture->lifecycle,
        &spec, out_slot);
}

static bool despawn_player(Fixture *fixture, HTHPlayerSlot slot)
{
    return hth_player_runtime_despawn(
        fixture->entities, fixture->actors, fixture->spatial,
        fixture->bodies, fixture->health, &fixture->lifecycle, slot);
}

static bool resolve_player(const Fixture *fixture, HTHPlayerSlot slot,
                           HTHEntityHandle *out_player)
{
    const HTHPlayerRoster *roster =
        hth_player_lifecycle_runtime_get_roster(&fixture->lifecycle);

    return hth_player_roster_get_slot(
        roster, fixture->entities, fixture->actors, slot, out_player);
}

static bool lifecycle_is_canonical(Fixture *fixture, HTHPlayerSlot slot)
{
    const HTHPlayerDefeatState *defeat;
    const HTHPlayerReviveWindow *window;
    double remaining;
    bool defeated;
    bool active;

    return hth_player_lifecycle_runtime_get_defeat(
               &fixture->lifecycle, fixture->entities, fixture->actors,
               slot, &defeat) &&
           hth_player_lifecycle_runtime_get_revive_window(
               &fixture->lifecycle, fixture->entities, fixture->actors,
               slot, &window) &&
           hth_player_defeat_is_defeated(defeat, &defeated) &&
           hth_player_revive_window_query(window, &active, &remaining) &&
           !defeated && !active && remaining == 0.0 &&
           !fixture->lifecycle.was_dead[slot];
}

static bool player_composition_is_current(const Fixture *fixture,
                                          HTHPlayerSlot slot)
{
    HTHEntityHandle player;

    return resolve_player(fixture, slot, &player) &&
           hth_entity_registry_is_alive(fixture->entities, player) &&
           hth_actor_store_has(fixture->actors, fixture->entities, player) &&
           hth_spatial_store_has(fixture->spatial, fixture->entities, player) &&
           hth_health_store_has(fixture->health, fixture->entities,
                                fixture->actors, player);
}

static bool test_spawn_nulls_and_output(void)
{
    Fixture fixture;
    HTHPlayerRuntimeSpawnSpec spec = player_spec(1.0F);
    HTHPlayerSlot slot;

    CHECK(fixture_init(&fixture));
#define CHECK_SPAWN_FAILURE(call) do {                                       \
    slot = 0U;                                                               \
    CHECK(!(call));                                                          \
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);                                  \
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);           \
    CHECK(hth_player_roster_count(                                           \
        hth_player_lifecycle_runtime_get_roster(                             \
            &fixture.lifecycle)) == 0U);                                     \
} while (0)
    CHECK_SPAWN_FAILURE(hth_player_runtime_spawn(
        NULL, fixture.actors, fixture.spatial, fixture.bodies,
        fixture.health, &fixture.lifecycle, &spec, &slot));
    CHECK_SPAWN_FAILURE(hth_player_runtime_spawn(
        fixture.entities, NULL, fixture.spatial, fixture.bodies,
        fixture.health, &fixture.lifecycle, &spec, &slot));
    CHECK_SPAWN_FAILURE(hth_player_runtime_spawn(
        fixture.entities, fixture.actors, NULL, fixture.bodies,
        fixture.health, &fixture.lifecycle, &spec, &slot));
    CHECK_SPAWN_FAILURE(hth_player_runtime_spawn(
        fixture.entities, fixture.actors, fixture.spatial, NULL,
        fixture.health, &fixture.lifecycle, &spec, &slot));
    CHECK_SPAWN_FAILURE(hth_player_runtime_spawn(
        fixture.entities, fixture.actors, fixture.spatial, fixture.bodies,
        NULL, &fixture.lifecycle, &spec, &slot));
    CHECK_SPAWN_FAILURE(hth_player_runtime_spawn(
        fixture.entities, fixture.actors, fixture.spatial, fixture.bodies,
        fixture.health, NULL, &spec, &slot));
    CHECK_SPAWN_FAILURE(hth_player_runtime_spawn(
        fixture.entities, fixture.actors, fixture.spatial, fixture.bodies,
        fixture.health, &fixture.lifecycle, NULL, &slot));
    CHECK(!hth_player_runtime_spawn(
        fixture.entities, fixture.actors, fixture.spatial, fixture.bodies,
        fixture.health, &fixture.lifecycle, &spec, NULL));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
#undef CHECK_SPAWN_FAILURE
    fixture_destroy(&fixture);
    return true;
}

static bool test_invalid_specs(void)
{
    Fixture fixture;
    HTHPlayerRuntimeSpawnSpec spec;
    HTHPlayerSlot slot;
    const float invalid_numbers[] = {NAN, INFINITY, -INFINITY};
    size_t coordinate;
    size_t index;

    CHECK(fixture_init(&fixture));
    for (index = 0U; index < sizeof(invalid_numbers) /
                               sizeof(invalid_numbers[0]); ++index) {
        for (coordinate = 0U; coordinate < 3U; ++coordinate) {
            spec = player_spec(2.0F);
            if (coordinate == 0U) {
                spec.transform.position.x = invalid_numbers[index];
            } else if (coordinate == 1U) {
                spec.transform.position.y = invalid_numbers[index];
            } else {
                spec.transform.position.z = invalid_numbers[index];
            }
            slot = 0U;
            CHECK(!spawn_player(&fixture, spec, &slot));
            CHECK(slot == HTH_PLAYER_SLOT_INVALID);
        }
        spec = player_spec(2.0F);
        spec.transform.yaw = invalid_numbers[index];
        slot = 0U;
        CHECK(!spawn_player(&fixture, spec, &slot));
        CHECK(slot == HTH_PLAYER_SLOT_INVALID);
        spec = player_spec(2.0F);
        spec.health.current = invalid_numbers[index];
        slot = 0U;
        CHECK(!spawn_player(&fixture, spec, &slot));
        CHECK(slot == HTH_PLAYER_SLOT_INVALID);
        spec = player_spec(2.0F);
        spec.health.maximum = invalid_numbers[index];
        slot = 0U;
        CHECK(!spawn_player(&fixture, spec, &slot));
        CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    }
    spec = player_spec(2.0F);
    spec.health.current = -1.0F;
    CHECK(!spawn_player(&fixture, spec, &slot));
    spec = player_spec(2.0F);
    spec.health.maximum = 0.0F;
    CHECK(!spawn_player(&fixture, spec, &slot));
    spec = player_spec(2.0F);
    spec.health.maximum = -1.0F;
    CHECK(!spawn_player(&fixture, spec, &slot));
    spec = player_spec(2.0F);
    spec.health.current = spec.health.maximum + 1.0F;
    CHECK(!spawn_player(&fixture, spec, &slot));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
    fixture_destroy(&fixture);
    return true;
}

static bool test_first_spawn_and_despawn(void)
{
    Fixture fixture;
    HTHPlayerRuntimeSpawnSpec spec = player_spec(3.0F);
    HTHSpatialTransform transform;
    HTHEntityHandle old_player;
    HTHHealth health;
    HTHPlayerSlot slot;

    CHECK(fixture_init(&fixture));
    CHECK(spawn_player(&fixture, spec, &slot));
    CHECK(slot == 0U);
    CHECK(player_composition_is_current(&fixture, slot));
    CHECK(resolve_player(&fixture, slot, &old_player));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                old_player, &transform));
    CHECK(transform.position.x == spec.transform.position.x);
    CHECK(transform.position.y == spec.transform.position.y);
    CHECK(transform.position.z == spec.transform.position.z);
    CHECK(transform.yaw == spec.transform.yaw);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, old_player, &health));
    CHECK(health.current == spec.health.current);
    CHECK(health.maximum == spec.health.maximum);
    CHECK(!hth_dynamic_body_has(fixture.bodies, fixture.entities,
                                old_player));
    CHECK(lifecycle_is_canonical(&fixture, slot));
    CHECK(despawn_player(&fixture, slot));
    CHECK(!hth_entity_registry_is_alive(fixture.entities, old_player));
    CHECK(hth_player_roster_count(
              hth_player_lifecycle_runtime_get_roster(
                  &fixture.lifecycle)) == 0U);
    CHECK(!fixture.lifecycle.defeat[slot].defeated);
    CHECK(!fixture.lifecycle.revive_windows[slot].active);
    CHECK(!fixture.lifecycle.was_dead[slot]);
    fixture_destroy(&fixture);
    return true;
}

static bool test_four_players_and_fifth_rollback(void)
{
    Fixture fixture;
    HTHEntityHandle players[HTH_MAX_PLAYERS];
    HTHPlayerSlot slots[HTH_MAX_PLAYERS];
    HTHPlayerSlot failed_slot = 0U;
    size_t index;

    CHECK(fixture_init(&fixture));
    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        CHECK(spawn_player(&fixture, player_spec((float)index),
                           &slots[index]));
        CHECK(slots[index] == index);
        CHECK(resolve_player(&fixture, slots[index], &players[index]));
        CHECK(lifecycle_is_canonical(&fixture, slots[index]));
    }
    CHECK(hth_entity_registry_live_count(fixture.entities) ==
          HTH_MAX_PLAYERS);
    CHECK(!spawn_player(&fixture, player_spec(10.0F), &failed_slot));
    CHECK(failed_slot == HTH_PLAYER_SLOT_INVALID);
    CHECK(hth_entity_registry_live_count(fixture.entities) ==
          HTH_MAX_PLAYERS);
    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        HTHEntityHandle current;

        CHECK(resolve_player(&fixture, slots[index], &current));
        CHECK(hth_entity_handle_equal(current, players[index]));
    }
    fixture_destroy(&fixture);
    return true;
}

static bool test_sparse_slot_and_generation_reuse(void)
{
    Fixture fixture;
    HTHEntityHandle before[HTH_MAX_PLAYERS];
    HTHEntityHandle replacement;
    HTHPlayerSlot slots[HTH_MAX_PLAYERS];
    HTHPlayerSlot replacement_slot;
    size_t index;

    CHECK(fixture_init(&fixture));
    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        CHECK(spawn_player(&fixture, player_spec((float)index),
                           &slots[index]));
        CHECK(resolve_player(&fixture, slots[index], &before[index]));
    }
    CHECK(despawn_player(&fixture, slots[1]));
    CHECK(spawn_player(&fixture, player_spec(20.0F), &replacement_slot));
    CHECK(replacement_slot == slots[1]);
    CHECK(resolve_player(&fixture, replacement_slot, &replacement));
    CHECK(replacement.index == before[1].index);
    CHECK(replacement.generation != before[1].generation);
    CHECK(!hth_entity_registry_is_alive(fixture.entities, before[1]));
    for (index = 2U; index < HTH_MAX_PLAYERS; ++index) {
        HTHEntityHandle current;

        CHECK(resolve_player(&fixture, slots[index], &current));
        CHECK(hth_entity_handle_equal(current, before[index]));
    }
    fixture_destroy(&fixture);
    return true;
}

static bool test_lifecycle_slot_cleanliness(void)
{
    Fixture fixture;
    HTHPlayerSlot slot;
    HTHPlayerSlot replacement_slot;

    CHECK(fixture_init(&fixture));
    CHECK(spawn_player(&fixture, player_spec(1.0F), &slot));
    CHECK(hth_player_defeat_mark(&fixture.lifecycle.defeat[slot]));
    CHECK(hth_player_revive_window_begin(
        &fixture.lifecycle.revive_windows[slot], 5.0));
    fixture.lifecycle.was_dead[slot] = true;
    CHECK(despawn_player(&fixture, slot));
    CHECK(!fixture.lifecycle.defeat[slot].defeated);
    CHECK(!fixture.lifecycle.revive_windows[slot].active);
    CHECK(fixture.lifecycle.revive_windows[slot].remaining_seconds == 0.0);
    CHECK(!fixture.lifecycle.was_dead[slot]);
    CHECK(spawn_player(&fixture, player_spec(2.0F), &replacement_slot));
    CHECK(replacement_slot == slot);
    CHECK(lifecycle_is_canonical(&fixture, replacement_slot));
    fixture_destroy(&fixture);
    return true;
}

static bool test_despawn_nulls_and_slots(void)
{
    Fixture fixture;
    HTHPlayerSlot slot;

    CHECK(fixture_init(&fixture));
    CHECK(spawn_player(&fixture, player_spec(1.0F), &slot));
#define CHECK_DESPAWN_FAILURE(call) do {                                     \
    CHECK(!(call));                                                          \
    CHECK(player_composition_is_current(&fixture, slot));                    \
} while (0)
    CHECK_DESPAWN_FAILURE(hth_player_runtime_despawn(
        NULL, fixture.actors, fixture.spatial, fixture.bodies,
        fixture.health, &fixture.lifecycle, slot));
    CHECK_DESPAWN_FAILURE(hth_player_runtime_despawn(
        fixture.entities, NULL, fixture.spatial, fixture.bodies,
        fixture.health, &fixture.lifecycle, slot));
    CHECK_DESPAWN_FAILURE(hth_player_runtime_despawn(
        fixture.entities, fixture.actors, NULL, fixture.bodies,
        fixture.health, &fixture.lifecycle, slot));
    CHECK_DESPAWN_FAILURE(hth_player_runtime_despawn(
        fixture.entities, fixture.actors, fixture.spatial, NULL,
        fixture.health, &fixture.lifecycle, slot));
    CHECK_DESPAWN_FAILURE(hth_player_runtime_despawn(
        fixture.entities, fixture.actors, fixture.spatial, fixture.bodies,
        NULL, &fixture.lifecycle, slot));
    CHECK_DESPAWN_FAILURE(hth_player_runtime_despawn(
        fixture.entities, fixture.actors, fixture.spatial, fixture.bodies,
        fixture.health, NULL, slot));
    CHECK_DESPAWN_FAILURE(despawn_player(&fixture,
                                         HTH_PLAYER_SLOT_INVALID));
    CHECK_DESPAWN_FAILURE(despawn_player(&fixture,
                                         HTH_MAX_PLAYERS + 1U));
    CHECK_DESPAWN_FAILURE(despawn_player(&fixture, 1U));
#undef CHECK_DESPAWN_FAILURE
    fixture_destroy(&fixture);
    return true;
}

static bool test_despawn_prevalidation(void)
{
    Fixture fixture;
    HTHPlayerRuntimeSpawnSpec spec = player_spec(4.0F);
    HTHSpatialTransform transform;
    HTHEntityHandle player;
    HTHHealth health;
    HTHPlayerSlot slot;

    CHECK(fixture_init(&fixture));
    CHECK(spawn_player(&fixture, spec, &slot));
    CHECK(resolve_player(&fixture, slot, &player));
    CHECK(hth_player_defeat_mark(&fixture.lifecycle.defeat[slot]));
    fixture.lifecycle.was_dead[slot] = true;

    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   player));
    CHECK(!despawn_player(&fixture, slot));
    CHECK(resolve_player(&fixture, slot, &player));
    CHECK(fixture.lifecycle.defeat[slot].defeated);
    CHECK(fixture.lifecycle.was_dead[slot]);
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   player, &spec.transform));

    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, player, &health));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, player));
    CHECK(!despawn_player(&fixture, slot));
    CHECK(resolve_player(&fixture, slot, &player));
    CHECK(fixture.lifecycle.defeat[slot].defeated);
    CHECK(fixture.lifecycle.was_dead[slot]);
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, player, health));

    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                player, &transform));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, player));
    CHECK(!despawn_player(&fixture, slot));
    CHECK(hth_player_roster_count(
              hth_player_lifecycle_runtime_get_roster(
                  &fixture.lifecycle)) == 1U);
    CHECK(fixture.lifecycle.defeat[slot].defeated);
    CHECK(fixture.lifecycle.was_dead[slot]);
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, player));
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities,
                                player, &transform));
    CHECK(despawn_player(&fixture, slot));
    fixture_destroy(&fixture);
    return true;
}

static bool test_stale_membership_fails(void)
{
    Fixture fixture;
    HTHEntityHandle player;
    HTHPlayerSlot slot;

    CHECK(fixture_init(&fixture));
    CHECK(spawn_player(&fixture, player_spec(1.0F), &slot));
    CHECK(resolve_player(&fixture, slot, &player));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, player));
    CHECK(!despawn_player(&fixture, slot));
    CHECK(hth_player_roster_count(
              hth_player_lifecycle_runtime_get_roster(
                  &fixture.lifecycle)) == 1U);
    fixture_destroy(&fixture);
    return true;
}

static bool test_bridge_coexistence(void)
{
    Fixture fixture;
    HTHPlayerBody body;
    HTHPlayerTargetBridge bridge;
    HTHSpatialTransform bridge_transform;
    HTHSpatialTransform generic_transform;
    HTHPlayerSlot bridge_slot;
    HTHPlayerSlot generic_slot;
    HTHEntityHandle bridge_entity;
    HTHEntityHandle generic_entity;

    CHECK(fixture_init(&fixture));
    bridge.target_entity = hth_entity_handle_invalid();
    CHECK(hth_player_body_init(&body, hth_vec3(1.0F, 0.0F, 2.0F)));
    CHECK(hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, fixture.health, &body,
        (HTHHealth){25.0F, 25.0F}));
    CHECK(hth_player_lifecycle_runtime_register(
        &fixture.lifecycle, fixture.entities, fixture.actors,
        bridge.target_entity, &bridge_slot));
    CHECK(bridge_slot == 0U);
    CHECK(spawn_player(&fixture, player_spec(7.0F), &generic_slot));
    CHECK(generic_slot == 1U);
    CHECK(resolve_player(&fixture, bridge_slot, &bridge_entity));
    CHECK(resolve_player(&fixture, generic_slot, &generic_entity));
    CHECK(!hth_entity_handle_equal(bridge_entity, generic_entity));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                bridge_entity, &bridge_transform));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                generic_entity, &generic_transform));
    CHECK(bridge_transform.position.x != generic_transform.position.x);
    CHECK(despawn_player(&fixture, generic_slot));
    CHECK(hth_player_lifecycle_runtime_unregister(
        &fixture.lifecycle, bridge_slot));
    CHECK(hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, fixture.health));
    fixture_destroy(&fixture);
    return true;
}

static bool test_multiple_component_independence(void)
{
    Fixture fixture;
    HTHSpatialTransform first_transform;
    HTHSpatialTransform second_transform;
    HTHHealth first_health;
    HTHHealth second_health;
    HTHDamageResult damage;
    HTHEntityHandle first;
    HTHEntityHandle second;
    HTHPlayerSlot first_slot;
    HTHPlayerSlot second_slot;

    CHECK(fixture_init(&fixture));
    CHECK(spawn_player(&fixture, player_spec(1.0F), &first_slot));
    CHECK(spawn_player(&fixture, player_spec(8.0F), &second_slot));
    CHECK(resolve_player(&fixture, first_slot, &first));
    CHECK(resolve_player(&fixture, second_slot, &second));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                first, &first_transform));
    second_transform = player_spec(30.0F).transform;
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities,
                                second, &second_transform));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                first, &second_transform));
    CHECK(second_transform.position.x == first_transform.position.x);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, first, &first_health));
    CHECK(hth_health_store_apply_damage(fixture.health, fixture.entities,
                                        fixture.actors, second, 3.0F,
                                        &damage));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, first, &second_health));
    CHECK(second_health.current == first_health.current);
    CHECK(second_health.maximum == first_health.maximum);
    fixture_destroy(&fixture);
    return true;
}

static bool test_repeated_spawn_despawn(void)
{
    Fixture fixture;
    size_t iteration;

    CHECK(fixture_init(&fixture));
    for (iteration = 0U; iteration < 128U; ++iteration) {
        HTHPlayerSlot slot;

        CHECK(spawn_player(&fixture,
                           player_spec((float)(iteration % 16U)), &slot));
        CHECK(slot == 0U);
        CHECK(player_composition_is_current(&fixture, slot));
        CHECK(lifecycle_is_canonical(&fixture, slot));
        CHECK(despawn_player(&fixture, slot));
        CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
    }
    fixture_destroy(&fixture);
    return true;
}

static bool test_interleaved_four_player_sequences(void)
{
    Fixture fixture;
    HTHPlayerSlot slots[HTH_MAX_PLAYERS];
    HTHPlayerSlot replacement;
    size_t index;

    CHECK(fixture_init(&fixture));
    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        CHECK(spawn_player(&fixture, player_spec((float)index),
                           &slots[index]));
    }
    CHECK(despawn_player(&fixture, slots[2]));
    CHECK(despawn_player(&fixture, slots[0]));
    CHECK(player_composition_is_current(&fixture, slots[1]));
    CHECK(player_composition_is_current(&fixture, slots[3]));
    CHECK(spawn_player(&fixture, player_spec(12.0F), &replacement));
    CHECK(replacement == slots[0]);
    CHECK(spawn_player(&fixture, player_spec(14.0F), &replacement));
    CHECK(replacement == slots[2]);
    CHECK(despawn_player(&fixture, slots[3]));
    CHECK(despawn_player(&fixture, slots[1]));
    CHECK(player_composition_is_current(&fixture, slots[0]));
    CHECK(player_composition_is_current(&fixture, slots[2]));
    CHECK(spawn_player(&fixture, player_spec(16.0F), &replacement));
    CHECK(replacement == slots[1]);
    CHECK(spawn_player(&fixture, player_spec(18.0F), &replacement));
    CHECK(replacement == slots[3]);
    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        CHECK(player_composition_is_current(&fixture, slots[index]));
        CHECK(lifecycle_is_canonical(&fixture, slots[index]));
    }
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    static bool (*const tests[])(void) = {
        test_spawn_nulls_and_output,
        test_invalid_specs,
        test_first_spawn_and_despawn,
        test_four_players_and_fifth_rollback,
        test_sparse_slot_and_generation_reuse,
        test_lifecycle_slot_cleanliness,
        test_despawn_nulls_and_slots,
        test_despawn_prevalidation,
        test_stale_membership_fails,
        test_bridge_coexistence,
        test_multiple_component_independence,
        test_repeated_spawn_despawn,
        test_interleaved_four_player_sequences
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return 1;
        }
    }
    puts("player runtime population tests passed");
    return 0;
}
