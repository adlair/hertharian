#include "enemy_decision.h"

#include "dynamic_body.h"
#include "enemy_target_selection.h"
#include "health.h"
#include "player_target_bridge.h"

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
    HTHEnemyTargetStore *targets;
    HTHSpatialStore *spatial;
    HTHHealthStore *health;
    HTHDynamicBodyStore *bodies;
} Fixture;

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->enemies = hth_enemy_store_create();
    fixture->targets = hth_enemy_target_store_create();
    fixture->spatial = hth_spatial_store_create();
    fixture->health = hth_health_store_create();
    fixture->bodies = hth_dynamic_body_store_create();
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->enemies != NULL && fixture->targets != NULL &&
           fixture->spatial != NULL && fixture->health != NULL &&
           fixture->bodies != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_enemy_target_store_destroy(fixture->targets);
    hth_health_store_destroy(fixture->health);
    hth_enemy_store_destroy(fixture->enemies);
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_spatial_store_destroy(fixture->spatial);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static HTHSpatialTransform transform(float x, float y, float z, float yaw)
{
    HTHSpatialTransform value = {{x, y, z}, yaw};

    return value;
}

static HTHCollisionWorld empty_world(void)
{
    HTHCollisionWorld world = {0};

    return world;
}

static HTHCollisionWorld blocking_world(void)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] = (HTHAABB){
        {0.75F, -0.25F, -0.25F},
        {1.25F, 0.25F, 0.25F}
    };
    world.obstacle_count = 1U;
    return world;
}

static bool create_entity(Fixture *fixture, HTHEntityHandle *out_entity)
{
    return hth_entity_registry_create_entity(fixture->entities, out_entity);
}

static bool create_spatial_entity(Fixture *fixture,
                                  HTHSpatialTransform value,
                                  HTHEntityHandle *out_entity)
{
    return create_entity(fixture, out_entity) &&
           hth_spatial_store_attach(fixture->spatial, fixture->entities,
                                    *out_entity, &value);
}

static bool create_enemy(Fixture *fixture, HTHSpatialTransform value,
                         HTHEntityHandle *out_enemy)
{
    return create_entity(fixture, out_enemy) &&
           hth_actor_store_attach(fixture->actors, fixture->entities,
                                  *out_enemy) &&
           hth_enemy_store_attach(fixture->enemies, fixture->entities,
                                  fixture->actors, *out_enemy) &&
           hth_spatial_store_attach(fixture->spatial, fixture->entities,
                                    *out_enemy, &value);
}

static bool set_target(Fixture *fixture, HTHEntityHandle enemy,
                       HTHEntityHandle target)
{
    return hth_enemy_target_store_set(
        fixture->targets, fixture->entities, fixture->actors,
        fixture->enemies, enemy, target);
}

static bool target_equals(Fixture *fixture, HTHEntityHandle enemy,
                          HTHEntityHandle expected)
{
    HTHEntityHandle actual;

    return hth_enemy_target_store_get(
               fixture->targets, fixture->entities, fixture->actors,
               fixture->enemies, enemy, &actual) &&
           hth_entity_handle_equal(actual, expected);
}

static bool evaluate(Fixture *fixture, const HTHCollisionWorld *world,
                     HTHEntityHandle enemy, float radius,
                     HTHEnemyIntent *out_intent)
{
    return hth_enemy_decision_evaluate(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->targets, fixture->spatial, world, enemy, radius,
        out_intent);
}

static bool evaluate_with_attack(Fixture *fixture,
                                 const HTHCollisionWorld *world,
                                 HTHEntityHandle enemy,
                                 float perception_radius,
                                 float attack_range,
                                 HTHEnemyIntent *out_intent)
{
    return hth_enemy_decision_evaluate_with_attack(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->targets, fixture->spatial, world, enemy,
        perception_radius, attack_range, out_intent);
}

static bool intent_is_idle(HTHEnemyIntent intent)
{
    return intent.kind == HTH_ENEMY_INTENT_IDLE &&
           hth_entity_handle_equal(intent.target,
                                   hth_entity_handle_invalid());
}

static bool intent_pursues(HTHEnemyIntent intent, HTHEntityHandle target)
{
    return intent.kind == HTH_ENEMY_INTENT_PURSUE &&
           hth_entity_handle_equal(intent.target, target);
}

static bool intent_attacks(HTHEnemyIntent intent, HTHEntityHandle target)
{
    return intent.kind == HTH_ENEMY_INTENT_ATTACK &&
           hth_entity_handle_equal(intent.target, target);
}

static bool test_attack_structural_failures_and_scalars(void)
{
    Fixture fixture;
    HTHCollisionWorld world = empty_world();
    const float invalid[] = {-1.0F, NAN, INFINITY, -INFINITY};
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEnemyIntent intent;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_spatial_entity(
        &fixture, transform(1.0F, 0.0F, 0.0F, 0.0F), &target));
    CHECK(set_target(&fixture, enemy, target));

#define CHECK_ATTACK_FAILURE(arguments)                                      \
    do {                                                                     \
        intent = (HTHEnemyIntent){HTH_ENEMY_INTENT_ATTACK, target};          \
        CHECK(!hth_enemy_decision_evaluate_with_attack arguments);           \
        CHECK(intent_is_idle(intent));                                        \
        CHECK(target_equals(&fixture, enemy, target));                        \
    } while (0)

    CHECK_ATTACK_FAILURE((NULL, fixture.actors, fixture.enemies,
                          fixture.targets, fixture.spatial, &world, enemy,
                          4.0F, 2.0F, &intent));
    CHECK_ATTACK_FAILURE((fixture.entities, NULL, fixture.enemies,
                          fixture.targets, fixture.spatial, &world, enemy,
                          4.0F, 2.0F, &intent));
    CHECK_ATTACK_FAILURE((fixture.entities, fixture.actors, NULL,
                          fixture.targets, fixture.spatial, &world, enemy,
                          4.0F, 2.0F, &intent));
    CHECK_ATTACK_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                          NULL, fixture.spatial, &world, enemy, 4.0F, 2.0F,
                          &intent));
    CHECK_ATTACK_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                          fixture.targets, NULL, &world, enemy, 4.0F, 2.0F,
                          &intent));
    CHECK_ATTACK_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                          fixture.targets, fixture.spatial, NULL, enemy,
                          4.0F, 2.0F, &intent));

    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        CHECK_ATTACK_FAILURE((fixture.entities, fixture.actors,
                              fixture.enemies, fixture.targets,
                              fixture.spatial, &world, enemy, invalid[index],
                              2.0F, &intent));
        CHECK_ATTACK_FAILURE((fixture.entities, fixture.actors,
                              fixture.enemies, fixture.targets,
                              fixture.spatial, &world, enemy, 4.0F,
                              invalid[index], &intent));
    }
#undef CHECK_ATTACK_FAILURE

    CHECK(!evaluate_with_attack(&fixture, &world, enemy, 4.0F, 2.0F, NULL));
    CHECK(target_equals(&fixture, enemy, target));
    fixture_destroy(&fixture);
    return true;
}

static bool test_attack_flow_boundaries_and_transitions(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHCollisionWorld blocked = blocking_world();
    HTHSpatialTransform position;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEnemyIntent historical;
    HTHEnemyIntent intent;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_spatial_entity(
        &fixture, transform(2.0F, 0.0F, 0.0F, 1.0F), &target));
    CHECK(!hth_actor_store_has(fixture.actors, fixture.entities, target));
    CHECK(set_target(&fixture, enemy, target));

    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 1.0F, 3.0F,
                               &intent));
    CHECK(intent_is_idle(intent));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 2.0F, 1.0F,
                               &intent));
    CHECK(intent_pursues(intent, target));
    CHECK(evaluate_with_attack(&fixture, &blocked, enemy, 2.0F, 1.0F,
                               &intent));
    CHECK(intent_is_idle(intent));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 2.0F, 2.0F,
                               &intent));
    CHECK(intent_attacks(intent, target));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 3.0F, 4.0F,
                               &intent));
    CHECK(intent_attacks(intent, target));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 3.0F, 0.0F,
                               &intent));
    CHECK(intent_pursues(intent, target));
    CHECK(evaluate_with_attack(&fixture, &blocked, enemy, 2.0F, 2.0F,
                               &intent));
    CHECK(intent_is_idle(intent));

    CHECK(evaluate(&fixture, &clear, enemy, 2.0F, &historical));
    CHECK(intent_pursues(historical, target));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 2.0F, 2.0F,
                               &intent));
    CHECK(intent_attacks(intent, target));

    position = transform(1.0F, 0.0F, 0.0F, -2.0F);
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, target,
                                &position));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 3.0F, 1.0F,
                               &intent));
    CHECK(intent_attacks(intent, target));
    position = transform(2.0F, 0.0F, 0.0F, 2.5F);
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, target,
                                &position));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 3.0F, 1.0F,
                               &intent));
    CHECK(intent_pursues(intent, target));
    position = transform(4.0F, 0.0F, 0.0F, 2.5F);
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, target,
                                &position));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 3.0F, 5.0F,
                               &intent));
    CHECK(intent_is_idle(intent));
    CHECK(target_equals(&fixture, enemy, target));
    fixture_destroy(&fixture);
    return true;
}

static bool test_zero_range_self_and_optional_state(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHHealth zero_health = {0.0F, 100.0F};
    HTHDynamicBody body = {{0.5F, 0.5F, 0.5F}, {3.0F, 2.0F, 1.0F}};
    HTHDynamicBody body_after;
    HTHSpatialTransform changed_yaw = transform(0.0F, 0.0F, 0.0F, 3.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle other_enemy;
    HTHEnemyIntent intent;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, -1.0F),
                       &enemy));
    CHECK(create_spatial_entity(
        &fixture, transform(0.0F, 0.0F, 0.0F, 2.0F), &target));
    CHECK(set_target(&fixture, enemy, target));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 0.0F, 0.0F,
                               &intent));
    CHECK(intent_attacks(intent, target));

    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, enemy, zero_health));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, enemy, &body));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 0.0F, 0.0F,
                               &intent));
    CHECK(intent_attacks(intent, target));
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities, enemy,
                               &body_after));
    CHECK(memcmp(&body, &body_after, sizeof(body)) == 0);
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, target,
                                &changed_yaw));
    for (index = 0U; index < 128U; ++index) {
        CHECK(evaluate_with_attack(&fixture, &clear, enemy, 0.0F, 0.0F,
                                   &intent));
        CHECK(intent_attacks(intent, target));
    }

    CHECK(set_target(&fixture, enemy, enemy));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 0.0F, 0.0F,
                               &intent));
    CHECK(intent_is_idle(intent));
    CHECK(target_equals(&fixture, enemy, enemy));

    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 1.0F),
                       &other_enemy));
    CHECK(set_target(&fixture, enemy, other_enemy));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 0.0F, 0.0F,
                               &intent));
    CHECK(intent_attacks(intent, other_enemy));
    fixture_destroy(&fixture);
    return true;
}

static bool test_attack_semantic_validation_and_generations(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle target_without_spatial;
    HTHEntityHandle stale_target;
    HTHEntityHandle reused_target;
    HTHEnemyIntent intent;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, origin, &enemy));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 4.0F, 2.0F,
                               &intent));
    CHECK(intent_is_idle(intent));

    CHECK(create_entity(&fixture, &target_without_spatial));
    CHECK(set_target(&fixture, enemy, target_without_spatial));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 4.0F, 2.0F,
                               &intent));
    CHECK(intent_is_idle(intent));

    CHECK(create_spatial_entity(&fixture, origin, &stale_target));
    CHECK(set_target(&fixture, enemy, stale_target));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale_target));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 4.0F, 2.0F,
                               &intent));
    CHECK(intent_is_idle(intent));
    CHECK(create_spatial_entity(&fixture, origin, &reused_target));
    CHECK(reused_target.index == stale_target.index);
    CHECK(reused_target.generation != stale_target.generation);
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 4.0F, 2.0F,
                               &intent));
    CHECK(intent_is_idle(intent));
    CHECK(!target_equals(&fixture, enemy, reused_target));

    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, enemy));
    CHECK(!evaluate_with_attack(&fixture, &clear, enemy, 4.0F, 2.0F,
                                &intent));
    CHECK(intent_is_idle(intent));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities, enemy,
                                   &origin));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities, enemy));
    CHECK(!evaluate_with_attack(&fixture, &clear, enemy, 4.0F, 2.0F,
                                &intent));
    CHECK(intent_is_idle(intent));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, enemy));
    CHECK(!evaluate_with_attack(&fixture, &clear, enemy, 4.0F, 2.0F,
                                &intent));
    CHECK(intent_is_idle(intent));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, enemy));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 4.0F, 2.0F,
                               &intent));
    CHECK(intent_is_idle(intent));
    fixture_destroy(&fixture);
    return true;
}

static bool test_player_target_bridge_attack_composition(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHCollisionWorld blocked = empty_world();
    HTHPlayerTargetBridge bridge = {hth_entity_handle_invalid()};
    HTHPlayerBody player;
    HTHPlayerBody player_before;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEnemyIntent intent;

    CHECK(fixture_create(&fixture));
    CHECK(hth_player_body_init(&player, (HTHVec3){6.0F, 0.0F, 0.0F}));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &target));
    CHECK(!hth_actor_store_has(fixture.actors, fixture.entities, target));
    CHECK(set_target(&fixture, enemy, target));

    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 5.0F, 3.0F,
                               &intent));
    CHECK(intent_is_idle(intent));
    player.position.x = 4.0F;
    CHECK(hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 5.0F, 3.0F,
                               &intent));
    CHECK(intent_pursues(intent, target));
    player.position.x = 2.0F;
    CHECK(hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    player_before = player;
    CHECK(evaluate_with_attack(&fixture, &clear, enemy, 5.0F, 3.0F,
                               &intent));
    CHECK(intent_attacks(intent, target));

    blocked.obstacles[0] = (HTHAABB){
        {0.75F, -2.0F, -2.0F},
        {1.25F, 2.0F, 2.0F}
    };
    blocked.obstacle_count = 1U;
    CHECK(evaluate_with_attack(&fixture, &blocked, enemy, 5.0F, 3.0F,
                               &intent));
    CHECK(intent_is_idle(intent));
    CHECK(memcmp(&player_before, &player, sizeof(player)) == 0);
    CHECK(target_equals(&fixture, enemy, target));
    CHECK(hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.spatial));
    fixture_destroy(&fixture);
    return true;
}

static bool test_structural_failures_and_canonical_output(void)
{
    Fixture fixture;
    HTHCollisionWorld world = empty_world();
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEnemyIntent intent;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_spatial_entity(
        &fixture, transform(1.0F, 0.0F, 0.0F, 0.0F), &target));
    CHECK(set_target(&fixture, enemy, target));

#define CHECK_FAILURE(arguments)                                             \
    do {                                                                     \
        intent.kind = HTH_ENEMY_INTENT_PURSUE;                               \
        intent.target = target;                                              \
        CHECK(!hth_enemy_decision_evaluate arguments);                       \
        CHECK(intent_is_idle(intent));                                        \
        CHECK(target_equals(&fixture, enemy, target));                        \
    } while (0)

    CHECK_FAILURE((NULL, fixture.actors, fixture.enemies, fixture.targets,
                   fixture.spatial, &world, enemy, 4.0F, &intent));
    CHECK_FAILURE((fixture.entities, NULL, fixture.enemies, fixture.targets,
                   fixture.spatial, &world, enemy, 4.0F, &intent));
    CHECK_FAILURE((fixture.entities, fixture.actors, NULL, fixture.targets,
                   fixture.spatial, &world, enemy, 4.0F, &intent));
    CHECK_FAILURE((fixture.entities, fixture.actors, fixture.enemies, NULL,
                   fixture.spatial, &world, enemy, 4.0F, &intent));
    CHECK_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                   fixture.targets, NULL, &world, enemy, 4.0F, &intent));
    CHECK_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                   fixture.targets, fixture.spatial, NULL, enemy, 4.0F,
                   &intent));
#undef CHECK_FAILURE

    CHECK(!evaluate(&fixture, &world, enemy, 4.0F, NULL));
    CHECK(target_equals(&fixture, enemy, target));
    fixture_destroy(&fixture);
    return true;
}

static bool test_invalid_radius(void)
{
    Fixture fixture;
    HTHCollisionWorld world = empty_world();
    const float radii[] = {-1.0F, NAN, INFINITY, -INFINITY};
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEnemyIntent intent;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_spatial_entity(
        &fixture, transform(1.0F, 0.0F, 0.0F, 0.0F), &target));
    CHECK(set_target(&fixture, enemy, target));
    for (index = 0U; index < sizeof(radii) / sizeof(radii[0]); ++index) {
        intent = (HTHEnemyIntent){HTH_ENEMY_INTENT_PURSUE, target};
        CHECK(!evaluate(&fixture, &world, enemy, radii[index], &intent));
        CHECK(intent_is_idle(intent));
        CHECK(target_equals(&fixture, enemy, target));
    }
    fixture_destroy(&fixture);
    return true;
}

static bool test_observer_requirements_and_generations(void)
{
    Fixture fixture;
    HTHCollisionWorld world = empty_world();
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle target;
    HTHEntityHandle entity_only;
    HTHEntityHandle actor_only;
    HTHEntityHandle enemy_without_spatial;
    HTHEntityHandle stale_enemy;
    HTHEntityHandle reused_enemy;
    HTHEnemyIntent intent;

    CHECK(fixture_create(&fixture));
    CHECK(create_spatial_entity(
        &fixture, transform(1.0F, 0.0F, 0.0F, 0.0F), &target));
    CHECK(!evaluate(&fixture, &world, hth_entity_handle_invalid(), 4.0F,
                    &intent));
    CHECK(intent_is_idle(intent));

    CHECK(create_entity(&fixture, &entity_only));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   entity_only, &origin));
    CHECK(!evaluate(&fixture, &world, entity_only, 4.0F, &intent));
    CHECK(intent_is_idle(intent));

    CHECK(create_entity(&fixture, &actor_only));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 actor_only));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   actor_only, &origin));
    CHECK(!evaluate(&fixture, &world, actor_only, 4.0F, &intent));
    CHECK(intent_is_idle(intent));

    CHECK(create_entity(&fixture, &enemy_without_spatial));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 enemy_without_spatial));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy_without_spatial));
    CHECK(!evaluate(&fixture, &world, enemy_without_spatial, 4.0F,
                    &intent));
    CHECK(intent_is_idle(intent));

    CHECK(create_enemy(&fixture, origin, &stale_enemy));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale_enemy));
    CHECK(!evaluate(&fixture, &world, stale_enemy, 4.0F, &intent));
    CHECK(intent_is_idle(intent));
    CHECK(create_enemy(&fixture, origin, &reused_enemy));
    CHECK(reused_enemy.index == stale_enemy.index);
    CHECK(reused_enemy.generation != stale_enemy.generation);
    CHECK(!evaluate(&fixture, &world, stale_enemy, 4.0F, &intent));
    CHECK(evaluate(&fixture, &world, reused_enemy, 4.0F, &intent));
    CHECK(intent_is_idle(intent));
    fixture_destroy(&fixture);
    return true;
}

static bool test_missing_stale_and_reused_target(void)
{
    Fixture fixture;
    HTHCollisionWorld world = empty_world();
    HTHEntityHandle enemy;
    HTHEntityHandle stale_target;
    HTHEntityHandle reused_target;
    HTHEnemyIntent intent;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(evaluate(&fixture, &world, enemy, 4.0F, &intent));
    CHECK(intent_is_idle(intent));

    CHECK(create_spatial_entity(
        &fixture, transform(1.0F, 0.0F, 0.0F, 0.0F), &stale_target));
    CHECK(set_target(&fixture, enemy, stale_target));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale_target));
    CHECK(evaluate(&fixture, &world, enemy, 4.0F, &intent));
    CHECK(intent_is_idle(intent));
    CHECK(create_spatial_entity(
        &fixture, transform(1.0F, 0.0F, 0.0F, 0.0F), &reused_target));
    CHECK(reused_target.index == stale_target.index);
    CHECK(reused_target.generation != stale_target.generation);
    CHECK(evaluate(&fixture, &world, enemy, 4.0F, &intent));
    CHECK(intent_is_idle(intent));
    CHECK(!target_equals(&fixture, enemy, reused_target));
    fixture_destroy(&fixture);
    return true;
}

static bool test_perception_radius_and_zero_length(void)
{
    Fixture fixture;
    HTHCollisionWorld world = empty_world();
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHSpatialTransform colocated = transform(0.0F, 0.0F, 0.0F, 2.0F);
    HTHEnemyIntent intent;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, -1.0F),
                       &enemy));
    CHECK(create_spatial_entity(
        &fixture, transform(3.0F, 4.0F, 0.0F, 1.0F), &target));
    CHECK(set_target(&fixture, enemy, target));
    CHECK(evaluate(&fixture, &world, enemy, 4.99F, &intent));
    CHECK(intent_is_idle(intent));
    CHECK(target_equals(&fixture, enemy, target));
    CHECK(evaluate(&fixture, &world, enemy, 5.0F, &intent));
    CHECK(intent_pursues(intent, target));
    CHECK(evaluate(&fixture, &world, enemy, 0.0F, &intent));
    CHECK(intent_is_idle(intent));
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, target,
                                &colocated));
    CHECK(evaluate(&fixture, &world, enemy, 0.0F, &intent));
    CHECK(intent_pursues(intent, target));
    CHECK(target_equals(&fixture, enemy, target));
    fixture_destroy(&fixture);
    return true;
}

static bool test_los_and_target_preservation(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHCollisionWorld blocked = blocking_world();
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEnemyIntent intent;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_spatial_entity(
        &fixture, transform(2.0F, 0.0F, 0.0F, 0.0F), &target));
    CHECK(set_target(&fixture, enemy, target));
    CHECK(evaluate(&fixture, &blocked, enemy, 3.0F, &intent));
    CHECK(intent_is_idle(intent));
    CHECK(target_equals(&fixture, enemy, target));
    CHECK(evaluate(&fixture, &clear, enemy, 3.0F, &intent));
    CHECK(intent_pursues(intent, target));
    CHECK(target_equals(&fixture, enemy, target));
    fixture_destroy(&fixture);
    return true;
}

static bool test_optional_roles_health_body_and_yaw(void)
{
    Fixture fixture;
    HTHCollisionWorld world = empty_world();
    HTHHealth zero_health = {0.0F, 100.0F};
    HTHDynamicBody body = {{0.5F, 0.5F, 0.5F}, {3.0F, 2.0F, 1.0F}};
    HTHSpatialTransform enemy_transform;
    HTHSpatialTransform target_transform;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle other_enemy;
    HTHEnemyIntent intent;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, -2.0F),
                       &enemy));
    CHECK(create_spatial_entity(
        &fixture, transform(1.0F, 0.0F, 0.0F, 2.0F), &target));
    CHECK(!hth_actor_store_has(fixture.actors, fixture.entities, target));
    CHECK(set_target(&fixture, enemy, target));
    CHECK(evaluate(&fixture, &world, enemy, 2.0F, &intent));
    CHECK(intent_pursues(intent, target));

    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, target));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, enemy, zero_health));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, target, zero_health));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, enemy, &body));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, target, &body));
    CHECK(evaluate(&fixture, &world, enemy, 2.0F, &intent));
    CHECK(intent_pursues(intent, target));

    CHECK(create_enemy(&fixture, transform(1.5F, 0.0F, 0.0F, 1.5F),
                       &other_enemy));
    CHECK(set_target(&fixture, enemy, other_enemy));
    CHECK(evaluate(&fixture, &world, enemy, 2.0F, &intent));
    CHECK(intent_pursues(intent, other_enemy));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &enemy_transform));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                other_enemy, &target_transform));
    CHECK(enemy_transform.yaw == -2.0F);
    CHECK(target_transform.yaw == 1.5F);
    fixture_destroy(&fixture);
    return true;
}

static bool test_determinism_and_complete_nonmutation(void)
{
    Fixture fixture;
    HTHCollisionWorld world = blocking_world();
    HTHCollisionWorld world_before;
    HTHHealth health_before = {50.0F, 75.0F};
    HTHHealth health_after;
    HTHDynamicBody body_before = {
        {0.5F, 0.75F, 1.0F},
        {1.0F, 2.0F, 3.0F}
    };
    HTHDynamicBody body_after;
    HTHSpatialTransform enemy_before = transform(
        0.0F, 0.0F, 0.0F, 0.75F);
    HTHSpatialTransform target_before = transform(
        2.0F, 0.0F, 0.0F, -0.5F);
    HTHSpatialTransform spatial_after;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEnemyIntent intent;
    size_t live_count;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, enemy_before, &enemy));
    CHECK(create_spatial_entity(&fixture, target_before, &target));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, target));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, enemy, health_before));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, enemy, &body_before));
    CHECK(set_target(&fixture, enemy, target));
    live_count = hth_entity_registry_live_count(fixture.entities);
    world_before = world;

    for (index = 0U; index < 32U; ++index) {
        CHECK(evaluate(&fixture, &world, enemy, 3.0F, &intent));
        CHECK(intent_is_idle(intent));
    }
    CHECK(hth_entity_registry_live_count(fixture.entities) == live_count);
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, enemy));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, target));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, enemy));
    CHECK(!hth_enemy_store_has(fixture.enemies, fixture.entities,
                               fixture.actors, target));
    CHECK(target_equals(&fixture, enemy, target));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &spatial_after));
    CHECK(memcmp(&enemy_before, &spatial_after, sizeof(enemy_before)) == 0);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &spatial_after));
    CHECK(memcmp(&target_before, &spatial_after, sizeof(target_before)) == 0);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, enemy, &health_after));
    CHECK(memcmp(&health_before, &health_after, sizeof(health_before)) == 0);
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities, enemy,
                               &body_after));
    CHECK(memcmp(&body_before, &body_after, sizeof(body_before)) == 0);
    CHECK(memcmp(&world_before, &world, sizeof(world)) == 0);
    fixture_destroy(&fixture);
    return true;
}

static bool test_selection_to_decision_composition(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHCollisionWorld blocked = blocking_world();
    HTHEntityHandle enemy;
    HTHEntityHandle candidate_a;
    HTHEntityHandle candidate_b;
    HTHEntityHandle candidates[2];
    HTHEntityHandle selected;
    HTHEnemyIntent intent;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_spatial_entity(
        &fixture, transform(2.0F, 0.0F, 0.0F, 0.0F), &candidate_a));
    CHECK(create_spatial_entity(
        &fixture, transform(0.0F, 4.0F, 0.0F, 0.0F), &candidate_b));
    candidates[0] = candidate_b;
    candidates[1] = candidate_a;
    CHECK(hth_enemy_target_select(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        &clear, fixture.targets, enemy, candidates, 2U, 5.0F, &selected));
    CHECK(hth_entity_handle_equal(selected, candidate_a));
    CHECK(evaluate(&fixture, &clear, enemy, 5.0F, &intent));
    CHECK(intent_pursues(intent, candidate_a));

    CHECK(evaluate(&fixture, &blocked, enemy, 5.0F, &intent));
    CHECK(intent_is_idle(intent));
    CHECK(target_equals(&fixture, enemy, candidate_a));

    CHECK(hth_entity_registry_destroy_entity(fixture.entities, candidate_a));
    CHECK(evaluate(&fixture, &clear, enemy, 5.0F, &intent));
    CHECK(intent_is_idle(intent));
    CHECK(!target_equals(&fixture, enemy, candidate_b));
    CHECK(hth_entity_registry_is_alive(fixture.entities, candidate_b));
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_attack_structural_failures_and_scalars,
        test_attack_flow_boundaries_and_transitions,
        test_zero_range_self_and_optional_state,
        test_attack_semantic_validation_and_generations,
        test_player_target_bridge_attack_composition,
        test_structural_failures_and_canonical_output,
        test_invalid_radius,
        test_observer_requirements_and_generations,
        test_missing_stale_and_reused_target,
        test_perception_radius_and_zero_length,
        test_los_and_target_preservation,
        test_optional_roles_health_body_and_yaw,
        test_determinism_and_complete_nonmutation,
        test_selection_to_decision_composition
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("enemy decision tests passed");
    return EXIT_SUCCESS;
}
