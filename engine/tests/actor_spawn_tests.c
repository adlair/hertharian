#include "actor_spawn.h"
#include "damage_intent.h"

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
    HTHSpatialStore *spatial;
    HTHDynamicBodyStore *bodies;
    HTHHealthStore *health;
} Fixture;

static bool fixture_create(Fixture *fixture)
{
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->spatial = hth_spatial_store_create();
    fixture->bodies = hth_dynamic_body_store_create();
    fixture->health = hth_health_store_create();
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

static HTHSpatialTransform test_transform(float offset)
{
    HTHSpatialTransform transform = {
        {1.0F + offset, 2.0F + offset, 3.0F + offset}, 0.25F + offset
    };

    return transform;
}

static HTHDynamicBody test_body(float offset)
{
    HTHDynamicBody body = {
        {0.5F + offset, 1.0F + offset, 0.75F + offset},
        {4.0F + offset, 5.0F + offset, 6.0F + offset}
    };

    return body;
}

static HTHHealth test_health(float offset)
{
    HTHHealth health = {75.0F + offset, 100.0F + offset};

    return health;
}

static bool vector_equal(HTHVec3 left, HTHVec3 right)
{
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

static bool transform_equal(HTHSpatialTransform left,
                            HTHSpatialTransform right)
{
    return vector_equal(left.position, right.position) &&
           left.yaw == right.yaw;
}

static bool body_equal(HTHDynamicBody left, HTHDynamicBody right)
{
    return vector_equal(left.half_extents, right.half_extents) &&
           vector_equal(left.velocity, right.velocity);
}

static bool health_equal(HTHHealth left, HTHHealth right)
{
    return left.current == right.current && left.maximum == right.maximum;
}

static bool handle_is_invalid(HTHEntityHandle handle)
{
    return hth_entity_handle_equal(handle, hth_entity_handle_invalid());
}

static bool spawn(Fixture *fixture, const HTHActorSpawnSpec *spec,
                  HTHEntityHandle *out_entity)
{
    return hth_actor_spawn(fixture->entities, fixture->actors,
                           fixture->spatial, fixture->bodies,
                           fixture->health, spec, out_entity);
}

static bool despawn(Fixture *fixture, HTHEntityHandle entity)
{
    return hth_actor_despawn(fixture->entities, fixture->actors,
                             fixture->spatial, fixture->bodies,
                             fixture->health, entity);
}

static bool composition_matches(const Fixture *fixture,
                                HTHEntityHandle entity,
                                const HTHActorSpawnSpec *spec)
{
    HTHSpatialTransform transform;
    HTHDynamicBody body;
    HTHHealth health;

    if (!hth_entity_registry_is_alive(fixture->entities, entity) ||
        !hth_actor_store_has(fixture->actors, fixture->entities, entity) ||
        hth_spatial_store_has(fixture->spatial, fixture->entities, entity) !=
            spec->has_spatial ||
        hth_dynamic_body_has(fixture->bodies, fixture->entities, entity) !=
            spec->has_body ||
        hth_health_store_has(fixture->health, fixture->entities,
                             fixture->actors, entity) != spec->has_health) {
        return false;
    }
    if (spec->has_spatial &&
        (!hth_spatial_store_get(fixture->spatial, fixture->entities, entity,
                                &transform) ||
         !transform_equal(transform, spec->transform))) {
        return false;
    }
    if (spec->has_body &&
        (!hth_dynamic_body_get(fixture->bodies, fixture->entities, entity,
                               &body) ||
         !body_equal(body, spec->body))) {
        return false;
    }
    if (spec->has_health &&
        (!hth_health_store_get(fixture->health, fixture->entities,
                               fixture->actors, entity, &health) ||
         !health_equal(health, spec->health))) {
        return false;
    }
    return true;
}

static bool composition_is_absent(const Fixture *fixture,
                                  HTHEntityHandle entity)
{
    return !hth_entity_registry_is_alive(fixture->entities, entity) &&
           !hth_actor_store_has(fixture->actors, fixture->entities, entity) &&
           !hth_spatial_store_has(fixture->spatial, fixture->entities,
                                  entity) &&
           !hth_dynamic_body_has(fixture->bodies, fixture->entities, entity) &&
           !hth_health_store_has(fixture->health, fixture->entities,
                                 fixture->actors, entity);
}

static HTHActorSpawnSpec full_spec(float offset)
{
    HTHActorSpawnSpec spec = {0};

    spec.has_spatial = true;
    spec.transform = test_transform(offset);
    spec.has_body = true;
    spec.body = test_body(offset);
    spec.has_health = true;
    spec.health = test_health(offset);
    return spec;
}

static bool expect_prevalidation_failure(Fixture *fixture,
                                         const HTHActorSpawnSpec *spec)
{
    const size_t live_before =
        hth_entity_registry_live_count(fixture->entities);
    HTHEntityHandle output = {7U, 9U};

    return !spawn(fixture, spec, &output) && handle_is_invalid(output) &&
           hth_entity_registry_live_count(fixture->entities) == live_before;
}

static bool test_valid_composition_matrix(void)
{
    Fixture fixture;
    HTHActorSpawnSpec specs[6] = {{0}};
    size_t index;

    CHECK(fixture_create(&fixture));
    specs[1].has_spatial = true;
    specs[1].transform = test_transform(1.0F);
    specs[2].has_health = true;
    specs[2].health = test_health(2.0F);
    specs[3].has_spatial = true;
    specs[3].transform = test_transform(3.0F);
    specs[3].has_health = true;
    specs[3].health = test_health(3.0F);
    specs[4].has_spatial = true;
    specs[4].transform = test_transform(4.0F);
    specs[4].has_body = true;
    specs[4].body = test_body(4.0F);
    specs[5] = full_spec(5.0F);

    for (index = 0U; index < sizeof(specs) / sizeof(specs[0]); ++index) {
        const size_t live_before =
            hth_entity_registry_live_count(fixture.entities);
        HTHEntityHandle entity = {91U, 17U};

        CHECK(spawn(&fixture, &specs[index], &entity));
        CHECK(!handle_is_invalid(entity));
        CHECK(hth_entity_registry_live_count(fixture.entities) ==
              live_before + 1U);
        CHECK(composition_matches(&fixture, entity, &specs[index]));
        CHECK(!specs[index].has_body || specs[index].has_spatial);
        CHECK(despawn(&fixture, entity));
        CHECK(composition_is_absent(&fixture, entity));
        CHECK(hth_entity_registry_live_count(fixture.entities) == live_before);
    }
    fixture_destroy(&fixture);
    return true;
}

static bool test_disabled_payloads_are_ignored(void)
{
    Fixture fixture;
    HTHActorSpawnSpec spec = {0};
    HTHEntityHandle entity;

    CHECK(fixture_create(&fixture));
    spec.transform = (HTHSpatialTransform){{NAN, INFINITY, -INFINITY}, NAN};
    spec.body = (HTHDynamicBody){{NAN, -1.0F, 0.0F},
                                 {INFINITY, NAN, -INFINITY}};
    spec.health = (HTHHealth){NAN, -INFINITY};
    CHECK(spawn(&fixture, &spec, &entity));
    CHECK(composition_matches(&fixture, entity, &spec));
    CHECK(despawn(&fixture, entity));
    fixture_destroy(&fixture);
    return true;
}

static bool test_invalid_composition_and_transform(void)
{
    Fixture fixture;
    HTHActorSpawnSpec spec = {0};
    HTHSpatialTransform invalid[] = {
        {{NAN, 2.0F, 3.0F}, 0.0F},
        {{1.0F, INFINITY, 3.0F}, 0.0F},
        {{1.0F, 2.0F, -INFINITY}, 0.0F},
        {{1.0F, 2.0F, 3.0F}, NAN},
        {{1.0F, 2.0F, 3.0F}, INFINITY}
    };
    size_t index;

    CHECK(fixture_create(&fixture));
    spec.has_body = true;
    spec.body = test_body(0.0F);
    CHECK(expect_prevalidation_failure(&fixture, &spec));
    spec.has_health = true;
    spec.health = test_health(0.0F);
    CHECK(expect_prevalidation_failure(&fixture, &spec));

    spec = (HTHActorSpawnSpec){0};
    spec.has_spatial = true;
    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        spec.transform = invalid[index];
        CHECK(expect_prevalidation_failure(&fixture, &spec));
    }
    fixture_destroy(&fixture);
    return true;
}

static bool test_invalid_body_and_health(void)
{
    Fixture fixture;
    HTHActorSpawnSpec spec = {0};
    HTHDynamicBody invalid_bodies[] = {
        {{0.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}},
        {{1.0F, -1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}},
        {{1.0F, 1.0F, NAN}, {0.0F, 0.0F, 0.0F}},
        {{INFINITY, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F}},
        {{1.0F, 1.0F, 1.0F}, {0.0F, NAN, 0.0F}},
        {{1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, INFINITY}}
    };
    HTHHealth invalid_health[] = {
        {0.0F, 0.0F}, {0.0F, -1.0F}, {0.0F, NAN},
        {0.0F, INFINITY}, {-1.0F, 10.0F}, {11.0F, 10.0F},
        {NAN, 10.0F}, {INFINITY, 10.0F}
    };
    size_t index;

    CHECK(fixture_create(&fixture));
    spec.has_spatial = true;
    spec.transform = test_transform(0.0F);
    spec.has_body = true;
    for (index = 0U;
         index < sizeof(invalid_bodies) / sizeof(invalid_bodies[0]); ++index) {
        spec.body = invalid_bodies[index];
        CHECK(expect_prevalidation_failure(&fixture, &spec));
    }

    spec = (HTHActorSpawnSpec){0};
    spec.has_health = true;
    for (index = 0U;
         index < sizeof(invalid_health) / sizeof(invalid_health[0]); ++index) {
        spec.health = invalid_health[index];
        CHECK(expect_prevalidation_failure(&fixture, &spec));
    }
    fixture_destroy(&fixture);
    return true;
}

static bool test_null_arguments_and_output_canonicalization(void)
{
    Fixture fixture;
    const HTHActorSpawnSpec spec = {0};
    HTHEntityHandle output = {4U, 8U};
    HTHEntityHandle entity;
    size_t live_before_null_output;

    CHECK(fixture_create(&fixture));
    live_before_null_output =
        hth_entity_registry_live_count(fixture.entities);
    CHECK(!hth_actor_spawn(NULL, fixture.actors, fixture.spatial,
                           fixture.bodies, fixture.health, &spec, &output));
    CHECK(handle_is_invalid(output));
    output = (HTHEntityHandle){4U, 8U};
    CHECK(!hth_actor_spawn(fixture.entities, NULL, fixture.spatial,
                           fixture.bodies, fixture.health, &spec, &output));
    CHECK(handle_is_invalid(output));
    output = (HTHEntityHandle){4U, 8U};
    CHECK(!hth_actor_spawn(fixture.entities, fixture.actors, NULL,
                           fixture.bodies, fixture.health, &spec, &output));
    CHECK(handle_is_invalid(output));
    output = (HTHEntityHandle){4U, 8U};
    CHECK(!hth_actor_spawn(fixture.entities, fixture.actors, fixture.spatial,
                           NULL, fixture.health, &spec, &output));
    CHECK(handle_is_invalid(output));
    output = (HTHEntityHandle){4U, 8U};
    CHECK(!hth_actor_spawn(fixture.entities, fixture.actors, fixture.spatial,
                           fixture.bodies, NULL, &spec, &output));
    CHECK(handle_is_invalid(output));
    output = (HTHEntityHandle){4U, 8U};
    CHECK(!hth_actor_spawn(fixture.entities, fixture.actors, fixture.spatial,
                           fixture.bodies, fixture.health, NULL, &output));
    CHECK(handle_is_invalid(output));
    CHECK(!hth_actor_spawn(fixture.entities, fixture.actors, fixture.spatial,
                           fixture.bodies, fixture.health, &spec, NULL));
    CHECK(hth_entity_registry_live_count(fixture.entities) ==
          live_before_null_output);

    CHECK(spawn(&fixture, &spec, &entity));
    CHECK(!hth_actor_despawn(NULL, fixture.actors, fixture.spatial,
                             fixture.bodies, fixture.health, entity));
    CHECK(!hth_actor_despawn(fixture.entities, NULL, fixture.spatial,
                             fixture.bodies, fixture.health, entity));
    CHECK(!hth_actor_despawn(fixture.entities, fixture.actors, NULL,
                             fixture.bodies, fixture.health, entity));
    CHECK(!hth_actor_despawn(fixture.entities, fixture.actors,
                             fixture.spatial, NULL, fixture.health, entity));
    CHECK(!hth_actor_despawn(fixture.entities, fixture.actors,
                             fixture.spatial, fixture.bodies, NULL, entity));
    CHECK(composition_matches(&fixture, entity, &spec));
    CHECK(despawn(&fixture, entity));
    fixture_destroy(&fixture);
    return true;
}

static bool test_despawn_current_state_optional_absence_and_unrelated(void)
{
    Fixture fixture;
    HTHActorSpawnSpec spatial_spec = {0};
    HTHActorSpawnSpec first_spec = full_spec(0.0F);
    HTHActorSpawnSpec second_spec = full_spec(10.0F);
    HTHEntityHandle current_state;
    HTHEntityHandle first;
    HTHEntityHandle second;

    CHECK(fixture_create(&fixture));
    spatial_spec.has_spatial = true;
    spatial_spec.transform = test_transform(20.0F);
    CHECK(spawn(&fixture, &spatial_spec, &current_state));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, current_state,
                                  (HTHHealth){40.0F, 50.0F}));
    CHECK(hth_health_store_has(fixture.health, fixture.entities,
                               fixture.actors, current_state));
    CHECK(despawn(&fixture, current_state));
    CHECK(!hth_entity_registry_is_alive(fixture.entities, current_state));
    CHECK(!hth_health_store_has(fixture.health, fixture.entities,
                                fixture.actors, current_state));

    CHECK(spawn(&fixture, &first_spec, &first));
    CHECK(spawn(&fixture, &second_spec, &second));
    CHECK(hth_dynamic_body_remove(fixture.bodies, fixture.entities, first));
    CHECK(despawn(&fixture, first));
    CHECK(!hth_entity_registry_is_alive(fixture.entities, first));
    CHECK(composition_matches(&fixture, second, &second_spec));
    CHECK(despawn(&fixture, second));
    fixture_destroy(&fixture);
    return true;
}

static bool test_stale_double_non_actor_and_removed_actor(void)
{
    Fixture fixture;
    HTHActorSpawnSpec spec = full_spec(0.0F);
    HTHEntityHandle stale;
    HTHEntityHandle replacement;
    HTHEntityHandle non_actor;
    HTHEntityHandle removed_actor;
    const HTHActorSpawnSpec actor_only = {0};

    CHECK(fixture_create(&fixture));
    CHECK(spawn(&fixture, &spec, &stale));
    CHECK(despawn(&fixture, stale));
    CHECK(!despawn(&fixture, stale));
    CHECK(spawn(&fixture, &spec, &replacement));
    CHECK(replacement.index == stale.index);
    CHECK(replacement.generation != stale.generation);
    CHECK(!despawn(&fixture, stale));
    CHECK(composition_matches(&fixture, replacement, &spec));

    CHECK(hth_entity_registry_create_entity(fixture.entities, &non_actor));
    CHECK(!despawn(&fixture, non_actor));
    CHECK(hth_entity_registry_is_alive(fixture.entities, non_actor));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, non_actor));

    CHECK(spawn(&fixture, &actor_only, &removed_actor));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities,
                                 removed_actor));
    CHECK(!despawn(&fixture, removed_actor));
    CHECK(hth_entity_registry_is_alive(fixture.entities, removed_actor));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities,
                                              removed_actor));
    CHECK(despawn(&fixture, replacement));
    fixture_destroy(&fixture);
    return true;
}

static bool test_zero_health_and_damage_intent_integration(void)
{
    Fixture fixture;
    const HTHActorSpawnSpec source_spec = {0};
    HTHActorSpawnSpec target_spec = {0};
    HTHActorSpawnSpec zero_health_spec = {0};
    HTHEntityHandle source;
    HTHEntityHandle target;
    HTHEntityHandle zero_health;
    HTHDamageIntent intent;
    HTHDamageResolution resolution;
    HTHHealth stored;

    CHECK(fixture_create(&fixture));
    zero_health_spec.has_health = true;
    zero_health_spec.health = (HTHHealth){0.0F, 10.0F};
    CHECK(spawn(&fixture, &zero_health_spec, &zero_health));
    CHECK(composition_matches(&fixture, zero_health, &zero_health_spec));

    target_spec.has_health = true;
    target_spec.health = (HTHHealth){100.0F, 100.0F};
    CHECK(spawn(&fixture, &source_spec, &source));
    CHECK(spawn(&fixture, &target_spec, &target));
    intent = (HTHDamageIntent){source, target, 25.0F};
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.applied && resolution.damage.previous == 100.0F &&
          resolution.damage.current == 75.0F &&
          resolution.damage.applied == 25.0F &&
          !resolution.damage.became_zero);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &stored));
    CHECK(health_equal(stored, (HTHHealth){75.0F, 100.0F}));
    CHECK(despawn(&fixture, source));
    CHECK(despawn(&fixture, target));
    CHECK(despawn(&fixture, zero_health));
    fixture_destroy(&fixture);
    return true;
}

static bool test_multiple_store_sets(void)
{
    Fixture first;
    Fixture second;
    HTHActorSpawnSpec first_spec = full_spec(0.0F);
    HTHActorSpawnSpec second_spec = full_spec(20.0F);
    HTHEntityHandle first_entity;
    HTHEntityHandle second_entity;

    CHECK(fixture_create(&first));
    CHECK(fixture_create(&second));
    CHECK(spawn(&first, &first_spec, &first_entity));
    CHECK(spawn(&second, &second_spec, &second_entity));
    CHECK(composition_matches(&first, first_entity, &first_spec));
    CHECK(composition_matches(&second, second_entity, &second_spec));
    CHECK(despawn(&first, first_entity));
    CHECK(composition_matches(&second, second_entity, &second_spec));
    CHECK(despawn(&second, second_entity));
    fixture_destroy(&second);
    fixture_destroy(&first);
    return true;
}

static bool test_growth(void)
{
    enum { ACTOR_COUNT = 96 };
    Fixture fixture;
    const HTHActorSpawnSpec spec = {0};
    HTHEntityHandle entities[ACTOR_COUNT];
    size_t index;

    CHECK(fixture_create(&fixture));
    for (index = 0U; index < ACTOR_COUNT; ++index) {
        CHECK(spawn(&fixture, &spec, &entities[index]));
    }
    CHECK(hth_entity_registry_live_count(fixture.entities) == ACTOR_COUNT);
    for (index = 0U; index < ACTOR_COUNT; ++index) {
        CHECK(composition_matches(&fixture, entities[index], &spec));
    }
    for (index = ACTOR_COUNT; index > 0U; --index) {
        CHECK(despawn(&fixture, entities[index - 1U]));
    }
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    const bool passed =
        test_valid_composition_matrix() &&
        test_disabled_payloads_are_ignored() &&
        test_invalid_composition_and_transform() &&
        test_invalid_body_and_health() &&
        test_null_arguments_and_output_canonicalization() &&
        test_despawn_current_state_optional_absence_and_unrelated() &&
        test_stale_double_non_actor_and_removed_actor() &&
        test_zero_health_and_damage_intent_integration() &&
        test_multiple_store_sets() && test_growth();

    if (!passed) {
        return EXIT_FAILURE;
    }
    puts("actor spawn foundation tests passed");
    return EXIT_SUCCESS;
}
