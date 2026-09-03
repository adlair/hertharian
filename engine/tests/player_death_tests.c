#include "player_death.h"

#include "damage_intent.h"
#include "enemy_pursuit_runtime.h"
#include "enemy_runtime_population.h"
#include "enemy_target.h"
#include "player_movement.h"
#include "player_target_bridge.h"

#include <float.h>
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
           fixture->spatial != NULL && fixture->bodies != NULL &&
           fixture->health != NULL && fixture->targets != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_enemy_target_store_destroy(fixture->targets);
    hth_health_store_destroy(fixture->health);
    hth_enemy_attack_cadence_store_destroy(fixture->cadences);
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_spatial_store_destroy(fixture->spatial);
    hth_enemy_store_destroy(fixture->enemies);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static bool create_actor(Fixture *fixture, HTHHealth health,
                         bool attach_health, HTHEntityHandle *out_entity)
{
    return hth_entity_registry_create_entity(fixture->entities, out_entity) &&
           hth_actor_store_attach(fixture->actors, fixture->entities,
                                  *out_entity) &&
           (!attach_health ||
            hth_health_store_attach(fixture->health, fixture->entities,
                                    fixture->actors, *out_entity, health));
}

static HTHPlayerTargetBridge inactive_bridge(void)
{
    HTHPlayerTargetBridge bridge = {hth_entity_handle_invalid()};

    return bridge;
}

static HTHCollisionWorld distant_world(void)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] = (HTHAABB){{100.0F, -10.0F, -10.0F},
                                   {101.0F, 10.0F, 10.0F}};
    world.obstacle_count = 1U;
    return world;
}

static bool player_equal(const HTHPlayerBody *left,
                         const HTHPlayerBody *right)
{
    return left->position.x == right->position.x &&
           left->position.y == right->position.y &&
           left->position.z == right->position.z &&
           left->velocity.x == right->velocity.x &&
           left->velocity.y == right->velocity.y &&
           left->velocity.z == right->velocity.z &&
           left->half_width == right->half_width &&
           left->height == right->height &&
           left->eye_height == right->eye_height &&
           left->grounded == right->grounded;
}

static bool spatial_equal(HTHSpatialTransform left,
                          HTHSpatialTransform right)
{
    return left.position.x == right.position.x &&
           left.position.y == right.position.y &&
           left.position.z == right.position.z && left.yaw == right.yaw;
}

static bool test_validation_generation_and_determinism(void)
{
    Fixture fixture;
    HTHEntityHandle entity_only;
    HTHEntityHandle actor_only;
    HTHEntityHandle stale;
    HTHEntityHandle replacement;
    bool dead = true;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &entity_only));
    CHECK(create_actor(&fixture, (HTHHealth){0}, false, &actor_only));
    CHECK(!hth_player_death_is_dead(NULL, fixture.actors, fixture.health,
                                    actor_only, &dead));
    CHECK(!dead);
    dead = true;
    CHECK(!hth_player_death_is_dead(fixture.entities, NULL, fixture.health,
                                    actor_only, &dead));
    CHECK(!dead);
    dead = true;
    CHECK(!hth_player_death_is_dead(fixture.entities, fixture.actors, NULL,
                                    actor_only, &dead));
    CHECK(!dead);
    CHECK(!hth_player_death_is_dead(fixture.entities, fixture.actors,
                                    fixture.health, actor_only, NULL));
    dead = true;
    CHECK(!hth_player_death_is_dead(
        fixture.entities, fixture.actors, fixture.health,
        hth_entity_handle_invalid(), &dead));
    CHECK(!dead);
    dead = true;
    CHECK(!hth_player_death_is_dead(fixture.entities, fixture.actors,
                                    fixture.health, entity_only, &dead));
    CHECK(!dead);
    dead = true;
    CHECK(!hth_player_death_is_dead(fixture.entities, fixture.actors,
                                    fixture.health, actor_only, &dead));
    CHECK(!dead);

    CHECK(create_actor(&fixture, (HTHHealth){FLT_MIN, 100.0F}, true,
                       &stale));
    for (index = 0U; index < 129U; ++index) {
        dead = true;
        CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                       fixture.health, stale, &dead));
        CHECK(!dead);
    }
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, stale));
    dead = true;
    CHECK(!hth_player_death_is_dead(fixture.entities, fixture.actors,
                                    fixture.health, stale, &dead));
    CHECK(!dead);
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, stale));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));
    dead = true;
    CHECK(!hth_player_death_is_dead(fixture.entities, fixture.actors,
                                    fixture.health, stale, &dead));
    CHECK(!dead);
    CHECK(hth_entity_registry_create_entity(fixture.entities, &replacement));
    CHECK(replacement.index == stale.index &&
          replacement.generation != stale.generation);
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 replacement));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, replacement,
                                  (HTHHealth){0.0F, 100.0F}));
    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, replacement, &dead));
    CHECK(dead);
    dead = true;
    CHECK(!hth_player_death_is_dead(fixture.entities, fixture.actors,
                                    fixture.health, stale, &dead));
    CHECK(!dead);
    fixture_destroy(&fixture);
    return true;
}

static bool test_health_damage_and_healing_semantics(void)
{
    Fixture fixture;
    HTHEntityHandle source;
    HTHEntityHandle positive;
    HTHEntityHandle lethal;
    HTHEntityHandle overkill;
    HTHEntityHandle initial_zero;
    HTHDamageIntent intent;
    HTHDamageResolution resolution;
    HTHHealingResult healing;
    bool dead;

    CHECK(fixture_create(&fixture));
    CHECK(create_actor(&fixture, (HTHHealth){0}, false, &source));
    CHECK(create_actor(&fixture, (HTHHealth){1.0F, 100.0F}, true,
                       &positive));
    CHECK(create_actor(&fixture, (HTHHealth){10.0F, 100.0F}, true,
                       &lethal));
    CHECK(create_actor(&fixture, (HTHHealth){10.0F, 100.0F}, true,
                       &overkill));
    CHECK(create_actor(&fixture, (HTHHealth){0.0F, 100.0F}, true,
                       &initial_zero));

    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, positive, &dead));
    CHECK(!dead);
    intent = (HTHDamageIntent){source, positive, 0.0F};
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, positive, &dead));
    CHECK(!dead);

    intent = (HTHDamageIntent){source, lethal, 10.0F};
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.damage.became_zero);
    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, lethal, &dead));
    CHECK(dead);
    intent.amount = 1.0F;
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(!resolution.damage.became_zero &&
          resolution.damage.applied == 0.0F);
    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, lethal, &dead));
    CHECK(dead);
    CHECK(hth_health_store_apply_healing(
        fixture.health, fixture.entities, fixture.actors, lethal, 1.0F,
        &healing));
    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, lethal, &dead));
    CHECK(!dead);

    intent = (HTHDamageIntent){source, overkill, 100.0F};
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, overkill, &dead));
    CHECK(dead);
    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, initial_zero, &dead));
    CHECK(dead);
    fixture_destroy(&fixture);
    return true;
}

static bool test_player_bridge_identity_and_movement_preservation(void)
{
    Fixture fixture;
    HTHPlayerTargetBridge bridge = inactive_bridge();
    HTHPlayerBody player;
    HTHPlayerBody player_before;
    HTHSpatialTransform spatial_before;
    HTHSpatialTransform spatial_after;
    HTHMovementConfig movement_config = hth_movement_config_default();
    HTHPlayerMovementIntent movement_intent = {
        {1.0F, 0.0F, 0.0F}, 1.0F, false
    };
    HTHPlayerMovementResult movement_result;
    HTHCollisionWorld world = distant_world();
    HTHEntityHandle player_target;
    HTHEntityHandle enemy;
    HTHEntityHandle retained_target;
    HTHDamageIntent intent;
    HTHDamageResolution resolution;
    bool dead;

    CHECK(fixture_create(&fixture));
    CHECK(hth_player_body_init(&player, (HTHVec3){0.0F, 0.0F, 0.0F}));
    player_before = player;
    CHECK(hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, fixture.health, &player,
        (HTHHealth){100.0F, 100.0F}));
    player_target = bridge.target_entity;
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                player_target, &spatial_before));
    CHECK(create_actor(&fixture, (HTHHealth){0}, false, &enemy));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy));
    CHECK(hth_enemy_target_store_set(fixture.targets, fixture.entities,
                                     fixture.actors, fixture.enemies, enemy,
                                     player_target));
    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, player_target, &dead));
    CHECK(!dead && player_equal(&player, &player_before));
    intent = (HTHDamageIntent){enemy, player_target, 100.0F};
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, player_target, &dead));
    CHECK(dead);
    CHECK(hth_entity_registry_is_alive(fixture.entities, player_target));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities,
                              player_target));
    CHECK(hth_spatial_store_has(fixture.spatial, fixture.entities,
                                player_target));
    CHECK(hth_health_store_has(fixture.health, fixture.entities,
                               fixture.actors, player_target));
    CHECK(!hth_enemy_store_has(fixture.enemies, fixture.entities,
                               fixture.actors, player_target));
    CHECK(!hth_dynamic_body_has(fixture.bodies, fixture.entities,
                                player_target));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                player_target, &spatial_after));
    CHECK(spatial_equal(spatial_before, spatial_after));
    CHECK(hth_entity_handle_equal(player_target, bridge.target_entity));
    CHECK(hth_enemy_target_store_get(fixture.targets, fixture.entities,
                                     fixture.actors, fixture.enemies, enemy,
                                     &retained_target));
    CHECK(hth_entity_handle_equal(retained_target, player_target));
    CHECK(player_equal(&player, &player_before));

    CHECK(hth_player_movement_step_with_result(
        &player, &world, &movement_config, &movement_intent, 0.05,
        &movement_result));
    CHECK(player.position.x != player_before.position.x);
    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, player_target, &dead));
    CHECK(dead);
    fixture_destroy(&fixture);
    return true;
}

static bool test_enemy_attack_runtime_composition(void)
{
    Fixture fixture;
    HTHPlayerTargetBridge bridge = inactive_bridge();
    HTHPlayerBody player;
    HTHEnemyRuntimeSpawnSpec enemy_spec = {
        {{0.5F, 0.9F, 0.0F}, 0.0F},
        {{0.3F, 0.9F, 0.3F}, {0.0F, 0.0F, 0.0F}},
        {100.0F, 100.0F}
    };
    HTHCollisionWorld world = distant_world();
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;
    HTHEntityHandle retained_target;
    HTHHealth health;
    bool dead;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(hth_player_body_init(&player, (HTHVec3){0.0F, 0.0F, 0.0F}));
    CHECK(hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, fixture.health, &player,
        (HTHHealth){100.0F, 100.0F}));
    candidate = bridge.target_entity;
    CHECK(hth_enemy_runtime_spawn(
        fixture.entities, fixture.actors, fixture.enemies, fixture.cadences,
        fixture.spatial, fixture.bodies, fixture.health, &enemy_spec,
        &enemy));
    for (index = 0U; index < 10U; ++index) {
        double delta_seconds = index == 0U ? 0.0 : 1.0;

        CHECK(hth_enemy_pursuit_runtime_step(
            fixture.entities, fixture.actors, fixture.enemies,
            fixture.spatial, fixture.bodies, fixture.health, fixture.targets,
            fixture.cadences, &world, &candidate, 1U, 8.0F, 1.25F, 2.0F,
            10.0F, 1.0, delta_seconds));
    }
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, candidate, &health));
    CHECK(health.current == 0.0F && health.maximum == 100.0F);
    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, candidate, &dead));
    CHECK(dead);
    CHECK(hth_enemy_target_store_get(fixture.targets, fixture.entities,
                                     fixture.actors, fixture.enemies, enemy,
                                     &retained_target));
    CHECK(hth_entity_handle_equal(retained_target, candidate));
    CHECK(hth_enemy_pursuit_runtime_step(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.bodies, fixture.health, fixture.targets, fixture.cadences,
        &world, &candidate, 1U, 8.0F, 1.25F, 2.0F, 10.0F, 1.0, 1.0));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, candidate, &health));
    CHECK(health.current == 0.0F);
    CHECK(hth_player_death_is_dead(fixture.entities, fixture.actors,
                                   fixture.health, candidate, &dead));
    CHECK(dead);
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    const struct {
        const char *name;
        bool (*run)(void);
    } tests[] = {
        {"validation/generation/determinism",
         test_validation_generation_and_determinism},
        {"Health/damage/healing semantics",
         test_health_damage_and_healing_semantics},
        {"Bridge identity/movement preservation",
         test_player_bridge_identity_and_movement_preservation},
        {"Enemy attack runtime composition",
         test_enemy_attack_runtime_composition}
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index].run()) {
            fprintf(stderr, "FAILED: %s\n", tests[index].name);
            return EXIT_FAILURE;
        }
        printf("PASS: %s\n", tests[index].name);
    }
    return EXIT_SUCCESS;
}
