#include "player_revive_interaction.h"

#include "player_death.h"

#include <float.h>
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

typedef struct Fixture {
    HTHEntityRegistry *entities;
    HTHActorStore *actors;
    HTHHealthStore *health;
    HTHSpatialStore *spatial;
    HTHPlayerLifecycleRuntime lifecycle;
    HTHEntityHandle players[HTH_MAX_PLAYERS];
    HTHPlayerSlot slots[HTH_MAX_PLAYERS];
    size_t player_count;
} Fixture;

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->health = hth_health_store_create();
    fixture->spatial = hth_spatial_store_create();
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->health != NULL && fixture->spatial != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_spatial_store_destroy(fixture->spatial);
    hth_health_store_destroy(fixture->health);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
    *fixture = (Fixture){0};
}

static bool add_player(Fixture *fixture, HTHVec3 position,
                       float current_health, bool with_spatial,
                       HTHPlayerSlot *out_slot)
{
    HTHSpatialTransform transform = {position, 0.0F};
    HTHEntityHandle entity;
    HTHPlayerSlot slot;

    if (fixture->player_count >= HTH_MAX_PLAYERS ||
        !hth_entity_registry_create_entity(fixture->entities, &entity) ||
        !hth_actor_store_attach(fixture->actors, fixture->entities, entity) ||
        !hth_health_store_attach(
            fixture->health, fixture->entities, fixture->actors, entity,
            (HTHHealth){current_health, 100.0F}) ||
        (with_spatial &&
         !hth_spatial_store_attach(
             fixture->spatial, fixture->entities, entity, &transform)) ||
        !hth_player_lifecycle_runtime_register(
            &fixture->lifecycle, fixture->entities, fixture->actors, entity,
            &slot)) {
        return false;
    }
    fixture->players[fixture->player_count] = entity;
    fixture->slots[fixture->player_count] = slot;
    fixture->player_count++;
    if (out_slot != NULL) {
        *out_slot = slot;
    }
    return true;
}

static bool set_position(Fixture *fixture, HTHPlayerSlot slot,
                         HTHVec3 position)
{
    HTHSpatialTransform transform = {position, 0.0F};

    return slot < fixture->player_count &&
           hth_spatial_store_set(fixture->spatial, fixture->entities,
                                 fixture->players[slot], &transform);
}

static bool begin_window(Fixture *fixture, HTHPlayerSlot slot)
{
    return slot < HTH_MAX_PLAYERS &&
           hth_player_revive_window_begin(
               &fixture->lifecycle.revive_windows[slot], 10.0);
}

static bool interaction_query(
    const HTHPlayerReviveInteraction *interaction,
    bool expected_active, HTHPlayerSlot expected_target,
    double expected_elapsed, double expected_required)
{
    bool active = !expected_active;
    HTHPlayerSlot target = 0U;
    double elapsed = -1.0;
    double required = -1.0;

    return hth_player_revive_interaction_query(
               interaction, &active, &target, &elapsed, &required) &&
           active == expected_active && target == expected_target &&
           elapsed == expected_elapsed && required == expected_required;
}

static bool same_interaction(HTHPlayerReviveInteraction left,
                             HTHPlayerReviveInteraction right)
{
    return left.target_slot == right.target_slot &&
           (left.elapsed_seconds == right.elapsed_seconds ||
            (isnan(left.elapsed_seconds) && isnan(right.elapsed_seconds))) &&
           (left.required_seconds == right.required_seconds ||
            (isnan(left.required_seconds) && isnan(right.required_seconds))) &&
           left.active == right.active;
}

static bool step(Fixture *fixture, HTHPlayerReviveInteraction *interaction,
                 HTHPlayerSlot reviver, HTHPlayerSlot target, bool held,
                 double delta, float range, double duration,
                 float revive_health, bool *out_revived)
{
    return hth_player_revive_interaction_step(
        interaction, &fixture->lifecycle, fixture->entities,
        fixture->actors, fixture->health, fixture->spatial, reviver, target,
        held, delta, range, duration, revive_health, out_revived);
}

static bool create_eligible_pair(Fixture *fixture,
                                 HTHPlayerSlot *out_reviver,
                                 HTHPlayerSlot *out_target)
{
    return fixture_create(fixture) &&
           add_player(fixture, hth_vec3(0.0F, 0.0F, 0.0F), 100.0F, true,
                      out_reviver) &&
           add_player(fixture, hth_vec3(1.0F, 0.0F, 0.0F), 0.0F, true,
                      out_target) &&
           begin_window(fixture, *out_target);
}

static bool test_zero_reset_query_and_copy(void)
{
    HTHPlayerReviveInteraction interaction = {0};
    HTHPlayerReviveInteraction copy;
    bool active = true;
    HTHPlayerSlot target = 0U;
    double elapsed = 1.0;
    double required = 1.0;
    size_t index;

    CHECK(interaction_query(&interaction, false, HTH_PLAYER_SLOT_INVALID,
                            0.0, 0.0));
    for (index = 0U; index < 128U; ++index) {
        CHECK(interaction_query(&interaction, false,
                                HTH_PLAYER_SLOT_INVALID, 0.0, 0.0));
    }
    CHECK(!hth_player_revive_interaction_query(
        NULL, &active, &target, &elapsed, &required));
    CHECK(!active && target == HTH_PLAYER_SLOT_INVALID && elapsed == 0.0 &&
          required == 0.0);
    active = true;
    CHECK(!hth_player_revive_interaction_query(
        &interaction, NULL, &target, &elapsed, &required));
    CHECK(target == HTH_PLAYER_SLOT_INVALID && elapsed == 0.0 &&
          required == 0.0);
    CHECK(!hth_player_revive_interaction_query(
        &interaction, &active, NULL, &elapsed, &required));
    CHECK(!active && elapsed == 0.0 && required == 0.0);
    CHECK(!hth_player_revive_interaction_query(
        &interaction, &active, &target, NULL, &required));
    CHECK(!active && target == HTH_PLAYER_SLOT_INVALID && required == 0.0);
    CHECK(!hth_player_revive_interaction_query(
        &interaction, &active, &target, &elapsed, NULL));
    CHECK(!active && target == HTH_PLAYER_SLOT_INVALID && elapsed == 0.0);

    interaction = (HTHPlayerReviveInteraction){1U, 0.25, 1.0, true};
    copy = interaction;
    CHECK(interaction_query(&copy, true, 1U, 0.25, 1.0));
    hth_player_revive_interaction_reset(&copy);
    CHECK(interaction_query(&copy, false, HTH_PLAYER_SLOT_INVALID, 0.0, 0.0));
    CHECK(interaction_query(&interaction, true, 1U, 0.25, 1.0));
    hth_player_revive_interaction_reset(NULL);
    return true;
}

static bool expect_malformed(HTHPlayerReviveInteraction interaction)
{
    HTHPlayerReviveInteraction before = interaction;
    bool active = true;
    HTHPlayerSlot target = 0U;
    double elapsed = 1.0;
    double required = 1.0;
    bool revived = true;

    return !hth_player_revive_interaction_query(
               &interaction, &active, &target, &elapsed, &required) &&
           !active && target == HTH_PLAYER_SLOT_INVALID && elapsed == 0.0 &&
           required == 0.0 &&
           !hth_player_revive_interaction_step(
               &interaction, NULL, NULL, NULL, NULL, NULL, 0U, 1U, false,
               0.0, 1.0F, 1.0, 1.0F, &revived) &&
           !revived && same_interaction(interaction, before);
}

static bool test_malformed_active_states(void)
{
    CHECK(expect_malformed((HTHPlayerReviveInteraction){
        HTH_PLAYER_SLOT_INVALID, 0.0, 1.0, true}));
    CHECK(expect_malformed((HTHPlayerReviveInteraction){0U, 0.0, 0.0, true}));
    CHECK(expect_malformed((HTHPlayerReviveInteraction){0U, 0.0, -1.0, true}));
    CHECK(expect_malformed((HTHPlayerReviveInteraction){0U, 0.0, NAN, true}));
    CHECK(expect_malformed((HTHPlayerReviveInteraction){0U, 0.0, INFINITY,
                                                        true}));
    CHECK(expect_malformed((HTHPlayerReviveInteraction){0U, -1.0, 1.0, true}));
    CHECK(expect_malformed((HTHPlayerReviveInteraction){0U, NAN, 1.0, true}));
    CHECK(expect_malformed((HTHPlayerReviveInteraction){0U, INFINITY, 1.0,
                                                        true}));
    CHECK(expect_malformed((HTHPlayerReviveInteraction){0U, 1.0, 1.0, true}));
    CHECK(expect_malformed((HTHPlayerReviveInteraction){0U, 2.0, 1.0, true}));
    return true;
}

static bool test_lifecycle_wrappers(void)
{
    Fixture fixture;
    HTHPlayerSlot reviver;
    HTHPlayerSlot target;
    HTHPlayerLifecycleRuntime runtime = {0};
    HTHHealth target_health;
    bool result = true;
    bool was_dead;

    CHECK(create_eligible_pair(&fixture, &reviver, &target));
#define CHECK_CAN_FAILURE(call)                                              \
    do {                                                                     \
        result = true;                                                       \
        CHECK(!(call));                                                      \
        CHECK(!result);                                                      \
    } while (0)
    CHECK_CAN_FAILURE(hth_player_lifecycle_runtime_can_revive(
        NULL, fixture.entities, fixture.actors, fixture.health, reviver,
        target, &result));
    CHECK_CAN_FAILURE(hth_player_lifecycle_runtime_can_revive(
        &fixture.lifecycle, NULL, fixture.actors, fixture.health, reviver,
        target, &result));
    CHECK_CAN_FAILURE(hth_player_lifecycle_runtime_can_revive(
        &fixture.lifecycle, fixture.entities, NULL, fixture.health, reviver,
        target, &result));
    CHECK_CAN_FAILURE(hth_player_lifecycle_runtime_can_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, NULL, reviver,
        target, &result));
    CHECK_CAN_FAILURE(hth_player_lifecycle_runtime_can_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, fixture.health,
        HTH_PLAYER_SLOT_INVALID, target, &result));
    CHECK_CAN_FAILURE(hth_player_lifecycle_runtime_can_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, fixture.health,
        reviver, HTH_PLAYER_SLOT_INVALID, &result));
    CHECK(!hth_player_lifecycle_runtime_can_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, fixture.health,
        reviver, target, NULL));
    CHECK_CAN_FAILURE(hth_player_lifecycle_runtime_can_revive(
        &runtime, fixture.entities, fixture.actors, fixture.health, 0U, 1U,
        &result));
#undef CHECK_CAN_FAILURE
    CHECK(hth_player_lifecycle_runtime_can_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, fixture.health,
        reviver, target, &result));
    CHECK(result);
    CHECK(hth_player_lifecycle_runtime_can_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, fixture.health,
        reviver, reviver, &result));
    CHECK(!result);

#define CHECK_EXECUTE_FAILURE(call)                                         \
    do {                                                                     \
        result = true;                                                       \
        CHECK(!(call));                                                      \
        CHECK(!result);                                                      \
    } while (0)
    CHECK_EXECUTE_FAILURE(hth_player_lifecycle_runtime_execute_revive(
        NULL, fixture.entities, fixture.actors, fixture.health, reviver,
        target, 25.0F, &result));
    CHECK_EXECUTE_FAILURE(hth_player_lifecycle_runtime_execute_revive(
        &fixture.lifecycle, NULL, fixture.actors, fixture.health, reviver,
        target, 25.0F, &result));
    CHECK_EXECUTE_FAILURE(hth_player_lifecycle_runtime_execute_revive(
        &fixture.lifecycle, fixture.entities, NULL, fixture.health, reviver,
        target, 25.0F, &result));
    CHECK_EXECUTE_FAILURE(hth_player_lifecycle_runtime_execute_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, NULL, reviver,
        target, 25.0F, &result));
    CHECK_EXECUTE_FAILURE(hth_player_lifecycle_runtime_execute_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, fixture.health,
        HTH_PLAYER_SLOT_INVALID, target, 25.0F, &result));
    CHECK_EXECUTE_FAILURE(hth_player_lifecycle_runtime_execute_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, fixture.health,
        reviver, HTH_PLAYER_SLOT_INVALID, 25.0F, &result));
    CHECK(!hth_player_lifecycle_runtime_execute_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, fixture.health,
        reviver, target, 25.0F, NULL));
#undef CHECK_EXECUTE_FAILURE

    was_dead = fixture.lifecycle.was_dead[target];
    CHECK(hth_player_lifecycle_runtime_execute_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, fixture.health,
        reviver, target, 25.0F, &result));
    CHECK(result && fixture.lifecycle.was_dead[target] == was_dead);
    CHECK(!fixture.lifecycle.defeat[reviver].defeated &&
          !fixture.lifecycle.defeat[target].defeated);
    CHECK(!fixture.lifecycle.revive_windows[target].active);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, fixture.players[target],
                               &target_health));
    CHECK(target_health.current == 25.0F);
    result = true;
    CHECK(hth_player_lifecycle_runtime_execute_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, fixture.health,
        reviver, target, 25.0F, &result));
    CHECK(!result && target_health.maximum == 100.0F);
    result = true;
    CHECK(!hth_player_lifecycle_runtime_execute_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, fixture.health,
        HTH_PLAYER_SLOT_INVALID, target, 25.0F, &result));
    CHECK(!result);
    result = true;
    CHECK(!hth_player_lifecycle_runtime_execute_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, fixture.health,
        reviver, target, NAN, &result));
    CHECK(!result);

    CHECK(hth_player_lifecycle_runtime_unregister(&fixture.lifecycle, target));
    result = true;
    CHECK(!hth_player_lifecycle_runtime_can_revive(
        &fixture.lifecycle, fixture.entities, fixture.actors, fixture.health,
        reviver, target, &result));
    CHECK(!result);
    fixture_destroy(&fixture);
    return true;
}

static bool test_step_pointers_idle_and_release(void)
{
    Fixture fixture;
    HTHPlayerSlot reviver;
    HTHPlayerSlot target;
    HTHPlayerReviveInteraction interaction = {0};
    HTHPlayerReviveInteraction active;
    bool revived = true;

    CHECK(create_eligible_pair(&fixture, &reviver, &target));
    CHECK(step(&fixture, &interaction, HTH_PLAYER_SLOT_INVALID,
               HTH_PLAYER_SLOT_INVALID, false, NAN, NAN, NAN, NAN,
               &revived));
    CHECK(!revived && interaction_query(
        &interaction, false, HTH_PLAYER_SLOT_INVALID, 0.0, 0.0));
    active = (HTHPlayerReviveInteraction){target, 0.25, 1.0, true};
    interaction = active;
    CHECK(step(&fixture, &interaction, HTH_PLAYER_SLOT_INVALID,
               HTH_PLAYER_SLOT_INVALID, false, NAN, NAN, NAN, NAN,
               &revived));
    CHECK(!revived && interaction_query(
        &interaction, false, HTH_PLAYER_SLOT_INVALID, 0.0, 0.0));

#define CHECK_POINTER_FAILURE(call)                                         \
    do {                                                                     \
        interaction = active;                                                \
        revived = true;                                                      \
        CHECK(!(call));                                                      \
        CHECK(!revived && same_interaction(interaction, active));             \
    } while (0)
    CHECK_POINTER_FAILURE(hth_player_revive_interaction_step(
        NULL, &fixture.lifecycle, fixture.entities, fixture.actors,
        fixture.health, fixture.spatial, reviver, target, true, 0.1, 2.0F,
        1.0, 10.0F, &revived));
    CHECK_POINTER_FAILURE(hth_player_revive_interaction_step(
        &interaction, NULL, fixture.entities, fixture.actors, fixture.health,
        fixture.spatial, reviver, target, true, 0.1, 2.0F, 1.0, 10.0F,
        &revived));
    CHECK_POINTER_FAILURE(hth_player_revive_interaction_step(
        &interaction, &fixture.lifecycle, NULL, fixture.actors,
        fixture.health, fixture.spatial, reviver, target, true, 0.1, 2.0F,
        1.0, 10.0F, &revived));
    CHECK_POINTER_FAILURE(hth_player_revive_interaction_step(
        &interaction, &fixture.lifecycle, fixture.entities, NULL,
        fixture.health, fixture.spatial, reviver, target, true, 0.1, 2.0F,
        1.0, 10.0F, &revived));
    CHECK_POINTER_FAILURE(hth_player_revive_interaction_step(
        &interaction, &fixture.lifecycle, fixture.entities, fixture.actors,
        NULL, fixture.spatial, reviver, target, true, 0.1, 2.0F, 1.0, 10.0F,
        &revived));
    CHECK_POINTER_FAILURE(hth_player_revive_interaction_step(
        &interaction, &fixture.lifecycle, fixture.entities, fixture.actors,
        fixture.health, NULL, reviver, target, true, 0.1, 2.0F, 1.0, 10.0F,
        &revived));
    interaction = active;
    CHECK(!hth_player_revive_interaction_step(
        &interaction, &fixture.lifecycle, fixture.entities, fixture.actors,
        fixture.health, fixture.spatial, reviver, target, true, 0.1, 2.0F,
        1.0, 10.0F, NULL));
    CHECK(same_interaction(interaction, active));
#undef CHECK_POINTER_FAILURE
    fixture_destroy(&fixture);
    return true;
}

static bool test_numeric_failures(void)
{
    static const double invalid_double[] = {-1.0, NAN, INFINITY, -INFINITY};
    static const float invalid_float[] = {0.0F, -1.0F, NAN, INFINITY,
                                          -INFINITY};
    Fixture fixture;
    HTHPlayerSlot reviver;
    HTHPlayerSlot target;
    HTHPlayerReviveInteraction interaction = {0};
    HTHPlayerReviveInteraction before;
    bool revived;
    size_t index;

    CHECK(create_eligible_pair(&fixture, &reviver, &target));
    interaction = (HTHPlayerReviveInteraction){target, 0.25, 1.0, true};
    before = interaction;
    for (index = 0U;
         index < sizeof(invalid_double) / sizeof(invalid_double[0]); ++index) {
        revived = true;
        CHECK(!step(&fixture, &interaction, reviver, target, true,
                    invalid_double[index], 2.0F, 1.0, 10.0F, &revived));
        CHECK(!revived && same_interaction(interaction, before));
    }
    for (index = 0U;
         index < sizeof(invalid_float) / sizeof(invalid_float[0]); ++index) {
        revived = true;
        CHECK(!step(&fixture, &interaction, reviver, target, true, 0.1,
                    invalid_float[index], 1.0, 10.0F, &revived));
        CHECK(!revived && same_interaction(interaction, before));
        revived = true;
        CHECK(!step(&fixture, &interaction, reviver, target, true, 0.1, 2.0F,
                    1.0, invalid_float[index], &revived));
        CHECK(!revived && same_interaction(interaction, before));
    }
    revived = true;
    CHECK(!step(&fixture, &interaction, reviver, target, true, 0.1, 2.0F,
                0.0, 10.0F, &revived));
    CHECK(!revived && same_interaction(interaction, before));
    revived = true;
    CHECK(!step(&fixture, &interaction, reviver, target, true, 0.1, 2.0F,
                -1.0, 10.0F, &revived));
    CHECK(!revived && same_interaction(interaction, before));
    revived = true;
    CHECK(!step(&fixture, &interaction, reviver, target, true, 0.1, 2.0F,
                NAN, 10.0F, &revived));
    CHECK(!revived && same_interaction(interaction, before));
    revived = true;
    CHECK(!step(&fixture, &interaction, reviver, target, true, 0.1, 2.0F,
                INFINITY, 10.0F, &revived));
    CHECK(!revived && same_interaction(interaction, before));
    revived = true;
    CHECK(!step(&fixture, &interaction, reviver, target, true, 0.1, 2.0F,
                -INFINITY, 10.0F, &revived));
    CHECK(!revived && same_interaction(interaction, before));
    revived = true;
    CHECK(!step(&fixture, &interaction, HTH_PLAYER_SLOT_INVALID, target, true,
                0.1, 2.0F, 1.0, 10.0F, &revived));
    CHECK(!revived && same_interaction(interaction, before));
    revived = true;
    CHECK(!step(&fixture, &interaction, reviver, HTH_PLAYER_SLOT_INVALID, true,
                0.1, 2.0F, 1.0, 10.0F, &revived));
    CHECK(!revived && same_interaction(interaction, before));
    fixture_destroy(&fixture);
    return true;
}

static bool test_spatial_range_and_policy(void)
{
    Fixture fixture;
    HTHPlayerSlot reviver;
    HTHPlayerSlot target;
    HTHPlayerReviveInteraction interaction = {0};
    HTHPlayerReviveInteraction before;
    bool revived = true;

    CHECK(create_eligible_pair(&fixture, &reviver, &target));
    CHECK(set_position(&fixture, reviver, hth_vec3(0.0F, 0.0F, 0.0F)));
    CHECK(set_position(&fixture, target, hth_vec3(3.0F, 4.0F, 0.0F)));
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.25, 5.0F,
               1.0, 10.0F, &revived));
    CHECK(!revived && interaction_query(&interaction, true, target, 0.25,
                                        1.0));
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.25, 4.99F,
               1.0, 10.0F, &revived));
    CHECK(!revived && interaction_query(
        &interaction, false, HTH_PLAYER_SLOT_INVALID, 0.0, 0.0));
    CHECK(set_position(&fixture, target, hth_vec3(0.0F, 5.1F, 0.0F)));
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.1, 5.0F,
               1.0, 10.0F, &revived));
    CHECK(!revived && !interaction.active);

    CHECK(set_position(&fixture, reviver,
                       hth_vec3(FLT_MAX, FLT_MAX, FLT_MAX)));
    CHECK(set_position(&fixture, target,
                       hth_vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX)));
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.1, FLT_MAX,
               1.0, 10.0F, &revived));
    CHECK(!revived && !interaction.active);

    CHECK(set_position(&fixture, reviver, hth_vec3(0.0F, 0.0F, 0.0F)));
    CHECK(set_position(&fixture, target, hth_vec3(0.0F, 0.0F, 0.0F)));
    CHECK(step(&fixture, &interaction, reviver, reviver, true, 0.1, FLT_MIN,
               1.0, 10.0F, &revived));
    CHECK(!revived && !interaction.active);
    before = (HTHPlayerReviveInteraction){target, 0.25, 1.0, true};
    interaction = before;
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   fixture.players[target]));
    CHECK(!step(&fixture, &interaction, reviver, target, true, 0.1, 2.0F,
                1.0, 10.0F, &revived));
    CHECK(!revived && same_interaction(interaction, before));
    fixture_destroy(&fixture);
    return true;
}

static bool test_progress_duration_and_target_change(void)
{
    Fixture fixture;
    HTHPlayerSlot reviver;
    HTHPlayerSlot target_a;
    HTHPlayerSlot target_b;
    HTHPlayerReviveInteraction interaction = {0};
    HTHPlayerReviveInteraction before;
    bool revived = true;

    CHECK(create_eligible_pair(&fixture, &reviver, &target_a));
    CHECK(add_player(&fixture, hth_vec3(1.5F, 0.0F, 0.0F), 0.0F, true,
                     &target_b));
    CHECK(begin_window(&fixture, target_b));
    CHECK(step(&fixture, &interaction, reviver, target_a, true, 0.25, 2.0F,
               1.0, 10.0F, &revived));
    CHECK(!revived && interaction_query(&interaction, true, target_a, 0.25,
                                        1.0));
    CHECK(step(&fixture, &interaction, reviver, target_a, true, 0.0, 2.0F,
               7.0, 10.0F, &revived));
    CHECK(interaction_query(&interaction, true, target_a, 0.25, 1.0));
    CHECK(step(&fixture, &interaction, reviver, target_a, true, 0.25, 2.0F,
               7.0, 10.0F, &revived));
    CHECK(interaction_query(&interaction, true, target_a, 0.5, 1.0));

    CHECK(step(&fixture, &interaction, reviver, target_b, true, 0.2, 2.0F,
               2.0, 10.0F, &revived));
    CHECK(!revived && interaction_query(&interaction, true, target_b, 0.2,
                                        2.0));
    fixture.lifecycle.defeat[target_a].defeated = true;
    CHECK(step(&fixture, &interaction, reviver, target_a, true, 0.1, 2.0F,
               1.0, 10.0F, &revived));
    CHECK(!revived && !interaction.active);

    fixture.lifecycle.defeat[target_a].defeated = false;
    interaction = (HTHPlayerReviveInteraction){target_a, 0.4, 1.0, true};
    before = interaction;
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   fixture.players[target_b]));
    CHECK(!step(&fixture, &interaction, reviver, target_b, true, 0.1, 2.0F,
                1.0, 10.0F, &revived));
    CHECK(!revived && same_interaction(interaction, before));
    fixture_destroy(&fixture);
    return true;
}

static bool test_completion_and_revive_health(void)
{
    Fixture fixture;
    HTHPlayerSlot reviver;
    HTHPlayerSlot target;
    HTHPlayerReviveInteraction interaction = {0};
    HTHHealth health;
    bool dead = true;
    bool revived = true;

    CHECK(create_eligible_pair(&fixture, &reviver, &target));
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.25, 2.0F,
               1.0, 5.0F, &revived));
    CHECK(!revived && interaction.active);
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.75, 2.0F,
               9.0, 20.0F, &revived));
    CHECK(revived && !interaction.active);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, fixture.players[target],
                               &health));
    CHECK(health.current == 20.0F);
    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, fixture.players[target],
                                   &dead));
    CHECK(!dead && !fixture.lifecycle.revive_windows[target].active &&
          !fixture.lifecycle.defeat[target].defeated);
    fixture_destroy(&fixture);

    CHECK(create_eligible_pair(&fixture, &reviver, &target));
    CHECK(step(&fixture, &interaction, reviver, target, true, 2.0, 2.0F,
               1.0, 10.0F, &revived));
    CHECK(revived && !interaction.active);
    fixture_destroy(&fixture);

    interaction = (HTHPlayerReviveInteraction){0};
    CHECK(create_eligible_pair(&fixture, &reviver, &target));
    CHECK(set_position(&fixture, target, hth_vec3(FLT_MIN, 0.0F, 0.0F)));
    CHECK(step(&fixture, &interaction, reviver, target, true, DBL_MIN,
               FLT_MIN, DBL_MIN, FLT_MIN, &revived));
    CHECK(revived && !interaction.active);
    fixture_destroy(&fixture);
    return true;
}

static bool test_cancellation_and_damage(void)
{
    Fixture fixture;
    HTHPlayerSlot reviver;
    HTHPlayerSlot target;
    HTHPlayerReviveInteraction interaction = {0};
    HTHDamageResult damage;
    bool revived = true;

    CHECK(create_eligible_pair(&fixture, &reviver, &target));
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.2, 2.0F,
               1.0, 10.0F, &revived));
    CHECK(hth_health_store_apply_damage(
        fixture.health, fixture.entities, fixture.actors,
        fixture.players[reviver], 10.0F, &damage));
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.2, 2.0F,
               1.0, 10.0F, &revived));
    CHECK(!revived && interaction_query(&interaction, true, target, 0.4,
                                        1.0));
    CHECK(hth_health_store_apply_damage(
        fixture.health, fixture.entities, fixture.actors,
        fixture.players[reviver], 100.0F, &damage));
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.2, 2.0F,
               1.0, 10.0F, &revived));
    CHECK(!revived && !interaction.active);

    CHECK(hth_health_store_apply_healing(
        fixture.health, fixture.entities, fixture.actors,
        fixture.players[reviver], 100.0F, &(HTHHealingResult){0}));
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.2, 2.0F,
               1.0, 10.0F, &revived));
    CHECK(interaction.active);
    hth_player_revive_window_reset(
        &fixture.lifecycle.revive_windows[target]);
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.2, 2.0F,
               1.0, 10.0F, &revived));
    CHECK(!revived && !interaction.active);
    fixture_destroy(&fixture);
    return true;
}

static bool test_residue_movement_and_defeat_cancellation(void)
{
    Fixture fixture;
    HTHPlayerSlot reviver;
    HTHPlayerSlot target;
    HTHPlayerReviveInteraction interaction = {0};
    bool revived = true;

    CHECK(create_eligible_pair(&fixture, &reviver, &target));
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.1, 2.0F,
               0.3, 10.0F, &revived));
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.1, 2.0F,
               9.0, 10.0F, &revived));
    CHECK(set_position(&fixture, target, hth_vec3(1.5F, 0.5F, 0.0F)));
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.09, 2.0F,
               9.0, 10.0F, &revived));
    CHECK(!revived && interaction.active &&
          interaction.elapsed_seconds < interaction.required_seconds);
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.02, 2.0F,
               9.0, 10.0F, &revived));
    CHECK(revived && !interaction.active);
    fixture_destroy(&fixture);

    interaction = (HTHPlayerReviveInteraction){0};
    CHECK(create_eligible_pair(&fixture, &reviver, &target));
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.2, 2.0F,
               1.0, 10.0F, &revived));
    fixture.lifecycle.defeat[reviver].defeated = true;
    CHECK(step(&fixture, &interaction, reviver, target, true, 0.2, 2.0F,
               1.0, 10.0F, &revived));
    CHECK(!revived && !interaction.active);
    fixture_destroy(&fixture);
    return true;
}

static bool test_multiple_revivers_and_determinism(void)
{
    Fixture fixture;
    HTHPlayerSlot reviver_a;
    HTHPlayerSlot target;
    HTHPlayerSlot reviver_b;
    HTHPlayerReviveInteraction interaction_a = {0};
    HTHPlayerReviveInteraction interaction_b = {0};
    HTHPlayerReviveInteraction idle = {0};
    bool revived = true;
    size_t index;

    CHECK(create_eligible_pair(&fixture, &reviver_a, &target));
    CHECK(add_player(&fixture, hth_vec3(0.5F, 0.0F, 0.0F), 100.0F, true,
                     &reviver_b));
    CHECK(step(&fixture, &interaction_a, reviver_a, target, true, 0.4, 2.0F,
               1.0, 10.0F, &revived));
    CHECK(step(&fixture, &interaction_b, reviver_b, target, true, 0.7, 2.0F,
               1.0, 10.0F, &revived));
    CHECK(interaction_query(&interaction_a, true, target, 0.4, 1.0));
    CHECK(interaction_query(&interaction_b, true, target, 0.7, 1.0));
    CHECK(step(&fixture, &interaction_a, reviver_a, target, true, 0.6, 2.0F,
               1.0, 10.0F, &revived));
    CHECK(revived && !interaction_a.active);
    revived = true;
    CHECK(step(&fixture, &interaction_b, reviver_b, target, true, 0.3, 2.0F,
               1.0, 10.0F, &revived));
    CHECK(!revived && !interaction_b.active);

    for (index = 0U; index < 128U; ++index) {
        HTHPlayerReviveInteraction repeated = idle;

        revived = true;
        CHECK(step(&fixture, &repeated, HTH_PLAYER_SLOT_INVALID,
                   HTH_PLAYER_SLOT_INVALID, false, NAN, NAN, NAN, NAN,
                   &revived));
        CHECK(!revived && same_interaction(repeated, idle));
    }
    fixture_destroy(&fixture);
    return true;
}

static bool test_stale_player_preserves_attempt(void)
{
    Fixture fixture;
    HTHPlayerSlot reviver;
    HTHPlayerSlot target;
    HTHPlayerReviveInteraction interaction;
    HTHPlayerReviveInteraction before;
    bool revived = true;

    CHECK(create_eligible_pair(&fixture, &reviver, &target));
    interaction = (HTHPlayerReviveInteraction){target, 0.25, 1.0, true};
    before = interaction;
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities,
                                 fixture.players[target]));
    CHECK(!step(&fixture, &interaction, reviver, target, true, 0.1, 2.0F,
                1.0, 10.0F, &revived));
    CHECK(!revived && same_interaction(interaction, before));
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_zero_reset_query_and_copy,
        test_malformed_active_states,
        test_lifecycle_wrappers,
        test_step_pointers_idle_and_release,
        test_numeric_failures,
        test_spatial_range_and_policy,
        test_progress_duration_and_target_change,
        test_completion_and_revive_health,
        test_cancellation_and_damage,
        test_residue_movement_and_defeat_cancellation,
        test_multiple_revivers_and_determinism,
        test_stale_player_preserves_attempt
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("player revive interaction tests passed");
    return EXIT_SUCCESS;
}
