#include "player_defeat.h"

#include "actor.h"
#include "entity.h"
#include "health.h"
#include "player_death.h"

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

static bool query_equals(const HTHPlayerDefeatState *state, bool expected)
{
    bool defeated = !expected;

    return hth_player_defeat_is_defeated(state, &defeated) &&
           defeated == expected;
}

static bool test_state_machine_and_validation(void)
{
    HTHPlayerDefeatState state = {0};
    bool defeated = true;
    size_t index;

    CHECK(query_equals(&state, false));
    for (index = 0U; index < 128U; ++index) {
        CHECK(query_equals(&state, false));
    }

    hth_player_defeat_reset(NULL);
    hth_player_defeat_reset(&state);
    CHECK(query_equals(&state, false));
    CHECK(!hth_player_defeat_mark(NULL));
    CHECK(hth_player_defeat_mark(&state));
    CHECK(query_equals(&state, true));
    for (index = 0U; index < 128U; ++index) {
        CHECK(hth_player_defeat_mark(&state));
        CHECK(query_equals(&state, true));
    }

    hth_player_defeat_reset(&state);
    CHECK(query_equals(&state, false));
    CHECK(!hth_player_defeat_is_defeated(NULL, &defeated));
    CHECK(!defeated);
    state.defeated = true;
    CHECK(!hth_player_defeat_is_defeated(&state, NULL));
    CHECK(state.defeated);
    return true;
}

static bool test_cycles_copy_and_distinct_instances(void)
{
    HTHPlayerDefeatState players[4] = {{0}};
    HTHPlayerDefeatState copy;
    size_t cycle;
    size_t index;

    for (cycle = 0U; cycle < 128U; ++cycle) {
        CHECK(hth_player_defeat_mark(&players[0]));
        CHECK(query_equals(&players[0], true));
        hth_player_defeat_reset(&players[0]);
        CHECK(query_equals(&players[0], false));
    }

    CHECK(hth_player_defeat_mark(&players[0]));
    CHECK(hth_player_defeat_mark(&players[2]));
    for (index = 0U; index < 4U; ++index) {
        CHECK(query_equals(&players[index], index == 0U || index == 2U));
    }

    copy = players[0];
    CHECK(query_equals(&copy, true));
    hth_player_defeat_reset(&copy);
    CHECK(query_equals(&copy, false));
    CHECK(query_equals(&players[0], true));
    return true;
}

static bool test_health_and_death_orthogonality(void)
{
    HTHEntityRegistry *entities = hth_entity_registry_create();
    HTHActorStore *actors = hth_actor_store_create();
    HTHHealthStore *health = hth_health_store_create();
    HTHPlayerDefeatState defeat = {0};
    HTHEntityHandle player;
    HTHDamageResult damage;
    HTHHealingResult healing;
    HTHHealth observed;
    bool dead;

    CHECK(entities != NULL && actors != NULL && health != NULL);
    CHECK(hth_entity_registry_create_entity(entities, &player));
    CHECK(hth_actor_store_attach(actors, entities, player));
    CHECK(hth_health_store_attach(
        health, entities, actors, player, (HTHHealth){10.0F, 10.0F}));

    CHECK(hth_player_death_is_dead(
        entities, actors, health, player, &dead) && !dead);
    CHECK(query_equals(&defeat, false));

    CHECK(hth_health_store_apply_damage(
        health, entities, actors, player, 10.0F, &damage));
    CHECK(hth_player_death_is_dead(
        entities, actors, health, player, &dead) && dead);
    CHECK(query_equals(&defeat, false));

    CHECK(hth_player_defeat_mark(&defeat));
    CHECK(query_equals(&defeat, true));
    CHECK(hth_player_death_is_dead(
        entities, actors, health, player, &dead) && dead);

    CHECK(hth_health_store_apply_healing(
        health, entities, actors, player, 5.0F, &healing));
    CHECK(hth_player_death_is_dead(
        entities, actors, health, player, &dead) && !dead);
    CHECK(query_equals(&defeat, true));
    CHECK(hth_health_store_get(health, entities, actors, player, &observed));
    CHECK(observed.current == 5.0F && observed.maximum == 10.0F);

    hth_player_defeat_reset(&defeat);
    CHECK(query_equals(&defeat, false));
    CHECK(hth_health_store_get(health, entities, actors, player, &observed));
    CHECK(observed.current == 5.0F);

    CHECK(hth_health_store_apply_damage(
        health, entities, actors, player, 5.0F, &damage));
    CHECK(hth_player_defeat_mark(&defeat));
    hth_player_defeat_reset(&defeat);
    CHECK(hth_player_death_is_dead(
        entities, actors, health, player, &dead) && dead);
    CHECK(query_equals(&defeat, false));

    CHECK(hth_health_store_apply_healing(
        health, entities, actors, player, 10.0F, &healing));
    CHECK(hth_player_defeat_mark(&defeat));
    CHECK(hth_player_death_is_dead(
        entities, actors, health, player, &dead) && !dead);
    CHECK(query_equals(&defeat, true));
    CHECK(hth_health_store_get(health, entities, actors, player, &observed));
    CHECK(observed.current == 10.0F);

    hth_health_store_destroy(health);
    hth_actor_store_destroy(actors);
    hth_entity_registry_destroy(entities);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_state_machine_and_validation,
        test_cycles_copy_and_distinct_instances,
        test_health_and_death_orthogonality
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("player defeat tests passed");
    return EXIT_SUCCESS;
}
