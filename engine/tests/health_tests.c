#include "health.h"

#include "dynamic_body.h"
#include "spatial.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
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
    HTHHealthStore *health;
} Fixture;

static bool fixture_create(Fixture *fixture)
{
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->health = hth_health_store_create();
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->health != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_health_store_destroy(fixture->health);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static bool create_actor(Fixture *fixture, HTHEntityHandle *out_entity)
{
    return hth_entity_registry_create_entity(fixture->entities, out_entity) &&
           hth_actor_store_attach(
               fixture->actors, fixture->entities, *out_entity);
}

static bool health_equal(HTHHealth left, HTHHealth right)
{
    return left.current == right.current && left.maximum == right.maximum;
}

static bool damage_result_zero(HTHDamageResult result)
{
    return result.previous == 0.0F && result.current == 0.0F &&
           result.applied == 0.0F && !result.became_zero;
}

static bool healing_result_zero(HTHHealingResult result)
{
    return result.previous == 0.0F && result.current == 0.0F &&
           result.applied == 0.0F;
}

static bool test_validity_attach_get_remove(void)
{
    Fixture fixture;
    HTHEntityHandle actor;
    HTHEntityHandle entity_only;
    HTHHealth output = {9.0F, 9.0F};
    const HTHHealth valid = {75.0F, 100.0F};
    const HTHHealth invalid[] = {
        {-1.0F, 100.0F}, {101.0F, 100.0F}, {0.0F, 0.0F},
        {0.0F, -1.0F}, {NAN, 100.0F}, {0.0F, NAN},
        {INFINITY, 100.0F}, {-INFINITY, 100.0F},
        {0.0F, INFINITY}, {0.0F, -INFINITY}
    };
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(create_actor(&fixture, &actor));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &entity_only));
    CHECK(hth_health_is_valid(valid));
    CHECK(hth_health_is_valid((HTHHealth){0.0F, 1.0F}));
    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        CHECK(!hth_health_is_valid(invalid[index]));
        CHECK(!hth_health_store_attach(fixture.health, fixture.entities,
                                       fixture.actors, actor,
                                       invalid[index]));
    }
    CHECK(!hth_health_store_attach(NULL, fixture.entities, fixture.actors,
                                   actor, valid));
    CHECK(!hth_health_store_attach(fixture.health, NULL, fixture.actors,
                                   actor, valid));
    CHECK(!hth_health_store_attach(fixture.health, fixture.entities, NULL,
                                   actor, valid));
    CHECK(!hth_health_store_attach(fixture.health, fixture.entities,
                                   fixture.actors, entity_only, valid));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, actor, valid));
    CHECK(!hth_health_store_attach(fixture.health, fixture.entities,
                                   fixture.actors, actor,
                                   (HTHHealth){1.0F, 2.0F}));
    CHECK(hth_health_store_has(fixture.health, fixture.entities,
                               fixture.actors, actor));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, actor, &output));
    CHECK(health_equal(output, valid));
    CHECK(!hth_health_store_has(NULL, fixture.entities, fixture.actors,
                                actor));
    CHECK(!hth_health_store_has(fixture.health, NULL, fixture.actors,
                                actor));
    CHECK(!hth_health_store_has(fixture.health, fixture.entities, NULL,
                                actor));
    CHECK(!hth_health_store_get(NULL, fixture.entities, fixture.actors,
                                actor, &output));
    CHECK(health_equal(output, (HTHHealth){0}));
    output = valid;
    CHECK(!hth_health_store_get(fixture.health, fixture.entities,
                                fixture.actors, actor, NULL));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, actor));
    CHECK(!hth_health_store_has(fixture.health, fixture.entities,
                                fixture.actors, actor));
    CHECK(!hth_health_store_remove(fixture.health, fixture.entities,
                                   fixture.actors, actor));
    CHECK(hth_entity_registry_is_alive(fixture.entities, actor));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, actor));
    fixture_destroy(&fixture);
    hth_health_store_destroy(NULL);
    return true;
}

static bool test_damage_semantics_and_failures(void)
{
    Fixture fixture;
    HTHEntityHandle actor;
    HTHEntityHandle exact_lethal;
    HTHEntityHandle overkill;
    HTHDamageResult result;
    HTHHealth health;

    CHECK(fixture_create(&fixture));
    CHECK(create_actor(&fixture, &actor));
    CHECK(create_actor(&fixture, &exact_lethal));
    CHECK(create_actor(&fixture, &overkill));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, actor,
                                  (HTHHealth){75.0F, 100.0F}));
    CHECK(hth_health_store_apply_damage(fixture.health, fixture.entities,
                                        fixture.actors, actor, 0.0F,
                                        &result));
    CHECK(result.previous == 75.0F && result.current == 75.0F &&
          result.applied == 0.0F && !result.became_zero);
    CHECK(hth_health_store_apply_damage(fixture.health, fixture.entities,
                                        fixture.actors, actor, 20.0F,
                                        &result));
    CHECK(result.previous == 75.0F && result.current == 55.0F &&
          result.applied == 20.0F && !result.became_zero);
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, exact_lethal,
                                  (HTHHealth){20.0F, 100.0F}));
    CHECK(hth_health_store_apply_damage(fixture.health, fixture.entities,
                                        fixture.actors, exact_lethal, 20.0F,
                                        &result));
    CHECK(result.previous == 20.0F && result.current == 0.0F &&
          result.applied == 20.0F && result.became_zero);
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, overkill,
                                  (HTHHealth){20.0F, 100.0F}));
    CHECK(hth_health_store_apply_damage(fixture.health, fixture.entities,
                                        fixture.actors, overkill, 1000.0F,
                                        &result));
    CHECK(result.previous == 20.0F && result.current == 0.0F &&
          result.applied == 20.0F && result.became_zero);
    CHECK(hth_health_store_apply_damage(fixture.health, fixture.entities,
                                        fixture.actors, overkill, 1.0F,
                                        &result));
    CHECK(result.previous == 0.0F && result.current == 0.0F &&
          result.applied == 0.0F && !result.became_zero);

    result = (HTHDamageResult){1.0F, 2.0F, 3.0F, true};
    CHECK(!hth_health_store_apply_damage(fixture.health, fixture.entities,
                                         fixture.actors, actor, -1.0F,
                                         &result));
    CHECK(damage_result_zero(result));
    result.previous = 1.0F;
    CHECK(!hth_health_store_apply_damage(fixture.health, fixture.entities,
                                         fixture.actors, actor, NAN,
                                         &result));
    CHECK(damage_result_zero(result));
    result.previous = 1.0F;
    CHECK(!hth_health_store_apply_damage(fixture.health, fixture.entities,
                                         fixture.actors, actor, INFINITY,
                                         &result));
    CHECK(damage_result_zero(result));
    result.previous = 1.0F;
    CHECK(!hth_health_store_apply_damage(fixture.health, fixture.entities,
                                         fixture.actors, actor, -INFINITY,
                                         &result));
    CHECK(damage_result_zero(result));
    CHECK(!hth_health_store_apply_damage(fixture.health, fixture.entities,
                                         fixture.actors, actor, 1.0F, NULL));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, actor, &health));
    CHECK(health_equal(health, (HTHHealth){55.0F, 100.0F}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_healing_saturation_and_maximum_immutability(void)
{
    Fixture fixture;
    HTHEntityHandle actor;
    HTHEntityHandle huge;
    HTHEntityHandle normal;
    HTHEntityHandle saturation;
    HTHDamageResult damage;
    HTHHealingResult result;
    HTHHealth health;

    CHECK(fixture_create(&fixture));
    CHECK(create_actor(&fixture, &actor));
    CHECK(create_actor(&fixture, &huge));
    CHECK(create_actor(&fixture, &normal));
    CHECK(create_actor(&fixture, &saturation));

    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, normal,
                                  (HTHHealth){50.0F, 100.0F}));
    CHECK(hth_health_store_apply_healing(fixture.health, fixture.entities,
                                         fixture.actors, normal, 25.0F,
                                         &result));
    CHECK(result.previous == 50.0F && result.current == 75.0F &&
          result.applied == 25.0F);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, normal, &health));
    CHECK(health_equal(health, (HTHHealth){75.0F, 100.0F}));

    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, saturation,
                                  (HTHHealth){80.0F, 100.0F}));
    CHECK(hth_health_store_apply_healing(fixture.health, fixture.entities,
                                         fixture.actors, saturation, 50.0F,
                                         &result));
    CHECK(result.previous == 80.0F && result.current == 100.0F &&
          result.applied == 20.0F);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, saturation, &health));
    CHECK(health_equal(health, (HTHHealth){100.0F, 100.0F}));

    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, actor,
                                  (HTHHealth){0.0F, 100.0F}));
    CHECK(hth_health_store_apply_healing(fixture.health, fixture.entities,
                                         fixture.actors, actor, 25.0F,
                                         &result));
    CHECK(result.previous == 0.0F && result.current == 25.0F &&
          result.applied == 25.0F);
    CHECK(hth_health_store_apply_healing(fixture.health, fixture.entities,
                                         fixture.actors, actor, 75.0F,
                                         &result));
    CHECK(result.previous == 25.0F && result.current == 100.0F &&
          result.applied == 75.0F);
    CHECK(hth_health_store_apply_healing(fixture.health, fixture.entities,
                                         fixture.actors, actor, 500.0F,
                                         &result));
    CHECK(result.previous == 100.0F && result.current == 100.0F &&
          result.applied == 0.0F);
    CHECK(hth_health_store_apply_damage(fixture.health, fixture.entities,
                                        fixture.actors, actor, 40.0F,
                                        &damage));
    CHECK(hth_health_store_apply_healing(fixture.health, fixture.entities,
                                         fixture.actors, actor, 0.0F,
                                         &result));
    CHECK(result.previous == 60.0F && result.current == 60.0F &&
          result.applied == 0.0F);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, actor, &health));
    CHECK(health.maximum == 100.0F);

    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, huge,
                                  (HTHHealth){1.0F, FLT_MAX}));
    CHECK(hth_health_store_apply_healing(fixture.health, fixture.entities,
                                         fixture.actors, huge, FLT_MAX,
                                         &result));
    CHECK(result.previous == 1.0F && result.current == FLT_MAX &&
          isfinite(result.current) && result.applied <= FLT_MAX);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, huge, &health));
    CHECK(health.maximum == FLT_MAX);

    result = (HTHHealingResult){1.0F, 2.0F, 3.0F};
    CHECK(!hth_health_store_apply_healing(fixture.health, fixture.entities,
                                          fixture.actors, actor, -1.0F,
                                          &result));
    CHECK(healing_result_zero(result));
    result.previous = 1.0F;
    CHECK(!hth_health_store_apply_healing(fixture.health, fixture.entities,
                                          fixture.actors, actor, NAN,
                                          &result));
    CHECK(healing_result_zero(result));
    result.previous = 1.0F;
    CHECK(!hth_health_store_apply_healing(fixture.health, fixture.entities,
                                          fixture.actors, actor, INFINITY,
                                          &result));
    CHECK(healing_result_zero(result));
    result.previous = 1.0F;
    CHECK(!hth_health_store_apply_healing(fixture.health, fixture.entities,
                                          fixture.actors, actor, -INFINITY,
                                          &result));
    CHECK(healing_result_zero(result));
    CHECK(!hth_health_store_apply_healing(fixture.health, fixture.entities,
                                          fixture.actors, actor, 1.0F, NULL));
    fixture_destroy(&fixture);
    return true;
}

static bool test_actor_dependency_entity_reuse_and_stale_handles(void)
{
    Fixture fixture;
    HTHEntityHandle stale;
    HTHEntityHandle replacement;
    HTHHealth output;
    HTHDamageResult damage;
    HTHHealingResult healing;

    CHECK(fixture_create(&fixture));
    CHECK(create_actor(&fixture, &stale));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, stale,
                                  (HTHHealth){40.0F, 80.0F}));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, stale));
    CHECK(!hth_health_store_has(fixture.health, fixture.entities,
                                fixture.actors, stale));
    CHECK(!hth_health_store_get(fixture.health, fixture.entities,
                                fixture.actors, stale, &output));
    CHECK(health_equal(output, (HTHHealth){0}));
    CHECK(!hth_health_store_apply_damage(fixture.health, fixture.entities,
                                         fixture.actors, stale, 5.0F,
                                         &damage));
    CHECK(!hth_health_store_apply_healing(fixture.health, fixture.entities,
                                          fixture.actors, stale, 5.0F,
                                          &healing));
    CHECK(!hth_health_store_remove(fixture.health, fixture.entities,
                                   fixture.actors, stale));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, stale));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, stale, &output));
    CHECK(health_equal(output, (HTHHealth){40.0F, 80.0F}));

    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));
    CHECK(!hth_health_store_has(fixture.health, fixture.entities,
                                fixture.actors, stale));
    CHECK(!hth_health_store_apply_damage(fixture.health, fixture.entities,
                                         fixture.actors, stale, 1.0F,
                                         &damage));
    CHECK(!hth_health_store_apply_healing(fixture.health, fixture.entities,
                                          fixture.actors, stale, 1.0F,
                                          &healing));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &replacement));
    CHECK(replacement.index == stale.index);
    CHECK(replacement.generation == stale.generation + 1U);
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 replacement));
    CHECK(!hth_health_store_has(fixture.health, fixture.entities,
                                fixture.actors, replacement));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, replacement,
                                  (HTHHealth){90.0F, 90.0F}));

    output = (HTHHealth){1.0F, 1.0F};
    CHECK(!hth_health_store_get(fixture.health, fixture.entities,
                                fixture.actors, stale, &output));
    CHECK(health_equal(output, (HTHHealth){0}));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, replacement, &output));
    CHECK(health_equal(output, (HTHHealth){90.0F, 90.0F}));

    CHECK(!hth_health_store_apply_damage(fixture.health, fixture.entities,
                                         fixture.actors, stale, 10.0F,
                                         &damage));
    CHECK(damage_result_zero(damage));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, replacement, &output));
    CHECK(health_equal(output, (HTHHealth){90.0F, 90.0F}));

    CHECK(!hth_health_store_apply_healing(fixture.health, fixture.entities,
                                          fixture.actors, stale, 10.0F,
                                          &healing));
    CHECK(healing_result_zero(healing));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, replacement, &output));
    CHECK(health_equal(output, (HTHHealth){90.0F, 90.0F}));

    CHECK(!hth_health_store_remove(fixture.health, fixture.entities,
                                   fixture.actors, stale));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, replacement, &output));
    CHECK(health_equal(output, (HTHHealth){90.0F, 90.0F}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_zero_health_transition_and_heal_from_zero(void)
{
    Fixture fixture;
    HTHEntityHandle actor;
    HTHDamageResult damage;
    HTHHealingResult healing;
    HTHHealth health;

    CHECK(fixture_create(&fixture));
    CHECK(create_actor(&fixture, &actor));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, actor,
                                  (HTHHealth){10.0F, 30.0F}));
    CHECK(hth_health_store_apply_damage(fixture.health, fixture.entities,
                                        fixture.actors, actor, 10.0F,
                                        &damage));
    CHECK(damage.became_zero);
    CHECK(hth_health_store_has(fixture.health, fixture.entities,
                               fixture.actors, actor));
    CHECK(hth_health_store_apply_damage(fixture.health, fixture.entities,
                                        fixture.actors, actor, 5.0F,
                                        &damage));
    CHECK(!damage.became_zero && damage.applied == 0.0F);
    CHECK(hth_health_store_apply_healing(fixture.health, fixture.entities,
                                         fixture.actors, actor, 3.0F,
                                         &healing));
    CHECK(healing.previous == 0.0F && healing.current == 3.0F &&
          healing.applied == 3.0F);
    CHECK(hth_health_store_apply_damage(fixture.health, fixture.entities,
                                        fixture.actors, actor, 3.0F,
                                        &damage));
    CHECK(damage.became_zero && damage.applied == 3.0F);
    CHECK(hth_entity_registry_is_alive(fixture.entities, actor));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, actor));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, actor, &health));
    CHECK(health_equal(health, (HTHHealth){0.0F, 30.0F}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_growth_sparse_iteration_and_mutation(void)
{
    enum { ENTITY_COUNT = 132 };
    static const uint32_t selected[] = {1U, 7U, 65U, 129U, 131U};
    Fixture fixture;
    HTHEntityHandle entities[ENTITY_COUNT];
    HTHHealthIterator iterator;
    HTHEntityHandle output_entity;
    HTHHealth output_health;
    HTHDamageResult damage;
    HTHHealingResult healing;
    size_t index;

    CHECK(fixture_create(&fixture));
    for (index = 0U; index < ENTITY_COUNT; ++index) {
        CHECK(hth_entity_registry_create_entity(fixture.entities,
                                                &entities[index]));
    }
    for (index = 0U; index < sizeof(selected) / sizeof(selected[0]); ++index) {
        uint32_t selected_index = selected[index];

        CHECK(entities[selected_index].index == selected_index);
        CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                     entities[selected_index]));
        CHECK(hth_health_store_attach(
            fixture.health, fixture.entities, fixture.actors,
            entities[selected_index],
            (HTHHealth){(float)selected_index + 10.0F, 300.0F}));
        if (selected_index == 1U) {
            CHECK(hth_health_store_apply_damage(
                fixture.health, fixture.entities, fixture.actors,
                entities[selected_index], 3.0F, &damage));
            CHECK(damage.previous == 11.0F && damage.current == 8.0F &&
                  damage.applied == 3.0F && !damage.became_zero);
        } else if (selected_index == 7U) {
            CHECK(hth_health_store_apply_healing(
                fixture.health, fixture.entities, fixture.actors,
                entities[selected_index], 5.0F, &healing));
            CHECK(healing.previous == 17.0F && healing.current == 22.0F &&
                  healing.applied == 5.0F);
        }
    }
    for (index = 0U; index < sizeof(selected) / sizeof(selected[0]); ++index) {
        uint32_t selected_index = selected[index];
        float expected = (float)selected_index + 10.0F;

        if (selected_index == 1U) {
            expected -= 3.0F;
        } else if (selected_index == 7U) {
            expected = 22.0F;
        }
        CHECK(hth_health_store_get(fixture.health, fixture.entities,
                                   fixture.actors, entities[selected_index],
                                   &output_health));
        CHECK(output_health.current == expected &&
              output_health.maximum == 300.0F);
    }

    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities,
                                 entities[65]));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities,
                                             entities[129]));
    hth_health_iterator_begin(&iterator);
    CHECK(hth_health_iterator_next(fixture.health, fixture.entities,
                                   fixture.actors, &iterator, &output_entity,
                                   &output_health));
    CHECK(output_entity.index == 1U && output_health.current == 8.0F);
    CHECK(hth_health_iterator_next(fixture.health, fixture.entities,
                                   fixture.actors, &iterator, &output_entity,
                                   &output_health));
    CHECK(output_entity.index == 7U && output_health.current == 22.0F &&
          output_health.maximum == 300.0F);
    CHECK(hth_health_iterator_next(fixture.health, fixture.entities,
                                   fixture.actors, &iterator, &output_entity,
                                   &output_health));
    CHECK(output_entity.index == 131U && output_health.current == 141.0F);
    CHECK(!hth_health_iterator_next(fixture.health, fixture.entities,
                                    fixture.actors, &iterator, &output_entity,
                                    &output_health));
    CHECK(hth_entity_handle_equal(output_entity,
                                  hth_entity_handle_invalid()));
    CHECK(health_equal(output_health, (HTHHealth){0}));

    hth_health_iterator_begin(NULL);
    CHECK(!hth_health_iterator_next(NULL, fixture.entities, fixture.actors,
                                    &iterator, &output_entity,
                                    &output_health));
    CHECK(!hth_health_iterator_next(fixture.health, NULL, fixture.actors,
                                    &iterator, &output_entity,
                                    &output_health));
    CHECK(!hth_health_iterator_next(fixture.health, fixture.entities, NULL,
                                    &iterator, &output_entity,
                                    &output_health));
    CHECK(!hth_health_iterator_next(fixture.health, fixture.entities,
                                    fixture.actors, NULL, &output_entity,
                                    &output_health));
    CHECK(!hth_health_iterator_next(fixture.health, fixture.entities,
                                    fixture.actors, &iterator, NULL,
                                    &output_health));
    CHECK(!hth_health_iterator_next(fixture.health, fixture.entities,
                                    fixture.actors, &iterator, &output_entity,
                                    NULL));
    fixture_destroy(&fixture);
    return true;
}

static bool test_component_composition_and_removal_independence(void)
{
    Fixture fixture;
    HTHSpatialStore *spatial = hth_spatial_store_create();
    HTHDynamicBodyStore *bodies = hth_dynamic_body_store_create();
    HTHEntityHandle entity;
    const HTHSpatialTransform transform = {{1.0F, 2.0F, 3.0F}, 0.5F};
    const HTHDynamicBody body = {{0.5F, 1.0F, 0.5F}, {1.0F, 0.0F, 2.0F}};
    HTHHealth output;

    CHECK(fixture_create(&fixture));
    CHECK(spatial != NULL && bodies != NULL);
    CHECK(create_actor(&fixture, &entity));
    CHECK(hth_spatial_store_attach(spatial, fixture.entities, entity,
                                   &transform));
    CHECK(hth_dynamic_body_attach(bodies, fixture.entities, spatial, entity,
                                  &body));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, entity,
                                  (HTHHealth){60.0F, 100.0F}));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, entity));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, entity));
    CHECK(hth_spatial_store_has(spatial, fixture.entities, entity));
    CHECK(hth_dynamic_body_has(bodies, fixture.entities, entity));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, entity,
                                  (HTHHealth){60.0F, 100.0F}));
    CHECK(hth_dynamic_body_remove(bodies, fixture.entities, entity));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, entity, &output));
    CHECK(hth_spatial_store_remove(spatial, fixture.entities, entity));
    CHECK(hth_health_store_has(fixture.health, fixture.entities,
                               fixture.actors, entity));
    hth_dynamic_body_store_destroy(bodies);
    hth_spatial_store_destroy(spatial);
    fixture_destroy(&fixture);
    return true;
}

static bool test_multiple_store_registry_triples(void)
{
    Fixture first;
    Fixture second;
    HTHEntityHandle first_entity;
    HTHEntityHandle second_entity;
    HTHHealth output;
    HTHDamageResult damage;

    CHECK(fixture_create(&first));
    CHECK(fixture_create(&second));
    CHECK(create_actor(&first, &first_entity));
    CHECK(create_actor(&second, &second_entity));
    CHECK(hth_entity_handle_equal(first_entity, second_entity));
    CHECK(hth_health_store_attach(first.health, first.entities, first.actors,
                                  first_entity,
                                  (HTHHealth){10.0F, 20.0F}));
    CHECK(hth_health_store_attach(second.health, second.entities,
                                  second.actors, second_entity,
                                  (HTHHealth){70.0F, 80.0F}));
    CHECK(hth_health_store_apply_damage(first.health, first.entities,
                                        first.actors, first_entity, 3.0F,
                                        &damage));
    CHECK(hth_health_store_get(first.health, first.entities, first.actors,
                               first_entity, &output));
    CHECK(health_equal(output, (HTHHealth){7.0F, 20.0F}));
    CHECK(hth_health_store_get(second.health, second.entities, second.actors,
                               second_entity, &output));
    CHECK(health_equal(output, (HTHHealth){70.0F, 80.0F}));
    fixture_destroy(&second);
    fixture_destroy(&first);
    return true;
}

int main(void)
{
    const bool passed =
        test_validity_attach_get_remove() &&
        test_damage_semantics_and_failures() &&
        test_healing_saturation_and_maximum_immutability() &&
        test_actor_dependency_entity_reuse_and_stale_handles() &&
        test_zero_health_transition_and_heal_from_zero() &&
        test_growth_sparse_iteration_and_mutation() &&
        test_component_composition_and_removal_independence() &&
        test_multiple_store_registry_triples();

    if (!passed) {
        return EXIT_FAILURE;
    }
    puts("health/damage foundation tests passed");
    return EXIT_SUCCESS;
}
