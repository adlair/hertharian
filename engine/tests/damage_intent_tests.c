#include "damage_intent.h"

#include "dynamic_body.h"
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

static bool resolution_is_zero(HTHDamageResolution resolution)
{
    return !resolution.applied && resolution.damage.previous == 0.0F &&
           resolution.damage.current == 0.0F &&
           resolution.damage.applied == 0.0F &&
           !resolution.damage.became_zero;
}

static bool get_health(const Fixture *fixture, HTHEntityHandle entity,
                       HTHHealth expected)
{
    HTHHealth actual;

    return hth_health_store_get(fixture->health, fixture->entities,
                                fixture->actors, entity, &actual) &&
           health_equal(actual, expected);
}

static bool test_validation_arguments_amounts_and_purity(void)
{
    Fixture fixture;
    HTHEntityHandle source;
    HTHEntityHandle target;
    HTHDamageIntent intent;
    HTHDamageResolution resolution;
    const float invalid_amounts[] = {-1.0F, NAN, INFINITY, -INFINITY};
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(create_actor(&fixture, &source));
    CHECK(create_actor(&fixture, &target));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, target,
                                  (HTHHealth){100.0F, 100.0F}));
    intent = (HTHDamageIntent){source, target, 10.0F};

    CHECK(hth_damage_intent_is_valid(&intent, fixture.entities,
                                     fixture.actors));
    CHECK(hth_damage_intent_is_valid(&intent, fixture.entities,
                                     fixture.actors));
    CHECK(get_health(&fixture, target, (HTHHealth){100.0F, 100.0F}));
    CHECK(!hth_damage_intent_is_valid(NULL, fixture.entities,
                                      fixture.actors));
    CHECK(!hth_damage_intent_is_valid(&intent, NULL, fixture.actors));
    CHECK(!hth_damage_intent_is_valid(&intent, fixture.entities, NULL));

    resolution = (HTHDamageResolution){true, {1.0F, 2.0F, 3.0F, true}};
    CHECK(!hth_damage_intent_resolve(NULL, fixture.entities, fixture.actors,
                                     fixture.health, &resolution));
    CHECK(resolution_is_zero(resolution));
    resolution.applied = true;
    CHECK(!hth_damage_intent_resolve(&intent, NULL, fixture.actors,
                                     fixture.health, &resolution));
    CHECK(resolution_is_zero(resolution));
    resolution.applied = true;
    CHECK(!hth_damage_intent_resolve(&intent, fixture.entities, NULL,
                                     fixture.health, &resolution));
    CHECK(resolution_is_zero(resolution));
    resolution.applied = true;
    CHECK(!hth_damage_intent_resolve(&intent, fixture.entities,
                                     fixture.actors, NULL, &resolution));
    CHECK(resolution_is_zero(resolution));
    CHECK(!hth_damage_intent_resolve(&intent, fixture.entities,
                                     fixture.actors, fixture.health, NULL));
    CHECK(get_health(&fixture, target, (HTHHealth){100.0F, 100.0F}));

    for (index = 0U;
         index < sizeof(invalid_amounts) / sizeof(invalid_amounts[0]);
         ++index) {
        intent.amount = invalid_amounts[index];
        CHECK(!hth_damage_intent_is_valid(&intent, fixture.entities,
                                          fixture.actors));
        resolution.applied = true;
        CHECK(!hth_damage_intent_resolve(
            &intent, fixture.entities, fixture.actors, fixture.health,
            &resolution));
        CHECK(resolution_is_zero(resolution));
        CHECK(get_health(&fixture, target, (HTHHealth){100.0F, 100.0F}));
    }
    fixture_destroy(&fixture);
    return true;
}

static bool test_resolution_matrix_source_without_health_and_zero(void)
{
    Fixture fixture;
    HTHEntityHandle source;
    HTHEntityHandle target;
    HTHDamageIntent intent;
    HTHDamageResolution resolution;

    CHECK(fixture_create(&fixture));
    CHECK(create_actor(&fixture, &source));
    CHECK(create_actor(&fixture, &target));
    intent = (HTHDamageIntent){source, target, 30.0F};

    CHECK(hth_damage_intent_is_valid(&intent, fixture.entities,
                                     fixture.actors));
    resolution = (HTHDamageResolution){true, {1.0F, 2.0F, 3.0F, true}};
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution_is_zero(resolution));

    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, target,
                                  (HTHHealth){100.0F, 100.0F}));
    CHECK(!hth_health_store_has(fixture.health, fixture.entities,
                                fixture.actors, source));
    intent.amount = 0.0F;
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.applied && resolution.damage.previous == 100.0F &&
          resolution.damage.current == 100.0F &&
          resolution.damage.applied == 0.0F &&
          !resolution.damage.became_zero);
    CHECK(get_health(&fixture, target, (HTHHealth){100.0F, 100.0F}));

    intent.amount = 30.0F;
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.applied && resolution.damage.previous == 100.0F &&
          resolution.damage.current == 70.0F &&
          resolution.damage.applied == 30.0F &&
          !resolution.damage.became_zero);
    CHECK(get_health(&fixture, target, (HTHHealth){70.0F, 100.0F}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_damage_edges_self_damage_and_repeated_resolution(void)
{
    Fixture fixture;
    HTHEntityHandle source;
    HTHEntityHandle lethal;
    HTHEntityHandle overkill;
    HTHEntityHandle already_zero;
    HTHEntityHandle self;
    HTHEntityHandle repeated;
    HTHDamageIntent intent;
    HTHDamageResolution resolution;

    CHECK(fixture_create(&fixture));
    CHECK(create_actor(&fixture, &source));
    CHECK(create_actor(&fixture, &lethal));
    CHECK(create_actor(&fixture, &overkill));
    CHECK(create_actor(&fixture, &already_zero));
    CHECK(create_actor(&fixture, &self));
    CHECK(create_actor(&fixture, &repeated));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, lethal,
                                  (HTHHealth){20.0F, 100.0F}));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, overkill,
                                  (HTHHealth){20.0F, 100.0F}));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, already_zero,
                                  (HTHHealth){0.0F, 100.0F}));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, self,
                                  (HTHHealth){50.0F, 50.0F}));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, repeated,
                                  (HTHHealth){100.0F, 100.0F}));

    intent = (HTHDamageIntent){source, lethal, 20.0F};
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.applied && resolution.damage.previous == 20.0F &&
          resolution.damage.current == 0.0F &&
          resolution.damage.applied == 20.0F &&
          resolution.damage.became_zero);
    CHECK(hth_entity_registry_is_alive(fixture.entities, lethal));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, lethal));
    CHECK(get_health(&fixture, lethal, (HTHHealth){0.0F, 100.0F}));

    intent = (HTHDamageIntent){source, overkill, 1000.0F};
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.applied && resolution.damage.previous == 20.0F &&
          resolution.damage.current == 0.0F &&
          resolution.damage.applied == 20.0F &&
          resolution.damage.became_zero);

    intent = (HTHDamageIntent){source, already_zero, 5.0F};
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.applied && resolution.damage.previous == 0.0F &&
          resolution.damage.current == 0.0F &&
          resolution.damage.applied == 0.0F &&
          !resolution.damage.became_zero);

    intent = (HTHDamageIntent){self, self, 10.0F};
    CHECK(hth_damage_intent_is_valid(&intent, fixture.entities,
                                     fixture.actors));
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.applied && resolution.damage.previous == 50.0F &&
          resolution.damage.current == 40.0F &&
          resolution.damage.applied == 10.0F &&
          !resolution.damage.became_zero);
    CHECK(get_health(&fixture, self, (HTHHealth){40.0F, 50.0F}));

    intent = (HTHDamageIntent){source, repeated, 10.0F};
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.damage.previous == 100.0F &&
          resolution.damage.current == 90.0F);
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.applied && resolution.damage.previous == 90.0F &&
          resolution.damage.current == 80.0F &&
          resolution.damage.applied == 10.0F &&
          !resolution.damage.became_zero);
    CHECK(get_health(&fixture, repeated, (HTHHealth){80.0F, 100.0F}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_source_and_target_death_generation_reuse(void)
{
    Fixture fixture;
    HTHEntityHandle stale_source;
    HTHEntityHandle source_replacement;
    HTHEntityHandle stable_target;
    HTHEntityHandle stable_source;
    HTHEntityHandle stale_target;
    HTHEntityHandle target_replacement;
    HTHDamageIntent source_intent;
    HTHDamageIntent target_intent;
    HTHDamageResolution resolution;

    CHECK(fixture_create(&fixture));
    CHECK(create_actor(&fixture, &stale_source));
    CHECK(create_actor(&fixture, &stable_target));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, stable_target,
                                  (HTHHealth){60.0F, 60.0F}));
    source_intent = (HTHDamageIntent){stale_source, stable_target, 10.0F};
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale_source));
    CHECK(!hth_damage_intent_is_valid(&source_intent, fixture.entities,
                                      fixture.actors));
    CHECK(!hth_damage_intent_resolve(&source_intent, fixture.entities,
                                     fixture.actors, fixture.health,
                                     &resolution));
    CHECK(resolution_is_zero(resolution));
    CHECK(get_health(&fixture, stable_target, (HTHHealth){60.0F, 60.0F}));
    CHECK(hth_entity_registry_create_entity(fixture.entities,
                                            &source_replacement));
    CHECK(source_replacement.index == stale_source.index);
    CHECK(source_replacement.generation == stale_source.generation + 1U);
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 source_replacement));
    CHECK(!hth_damage_intent_is_valid(&source_intent, fixture.entities,
                                      fixture.actors));
    CHECK(!hth_damage_intent_resolve(&source_intent, fixture.entities,
                                     fixture.actors, fixture.health,
                                     &resolution));
    CHECK(get_health(&fixture, stable_target, (HTHHealth){60.0F, 60.0F}));

    CHECK(create_actor(&fixture, &stable_source));
    CHECK(create_actor(&fixture, &stale_target));
    target_intent = (HTHDamageIntent){stable_source, stale_target, 10.0F};
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale_target));
    CHECK(!hth_damage_intent_is_valid(&target_intent, fixture.entities,
                                      fixture.actors));
    CHECK(!hth_damage_intent_resolve(&target_intent, fixture.entities,
                                     fixture.actors, fixture.health,
                                     &resolution));
    CHECK(hth_entity_registry_create_entity(fixture.entities,
                                            &target_replacement));
    CHECK(target_replacement.index == stale_target.index);
    CHECK(target_replacement.generation == stale_target.generation + 1U);
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 target_replacement));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, target_replacement,
                                  (HTHHealth){90.0F, 90.0F}));
    CHECK(!hth_damage_intent_is_valid(&target_intent, fixture.entities,
                                      fixture.actors));
    CHECK(!hth_damage_intent_resolve(&target_intent, fixture.entities,
                                     fixture.actors, fixture.health,
                                     &resolution));
    CHECK(resolution_is_zero(resolution));
    CHECK(get_health(&fixture, target_replacement,
                     (HTHHealth){90.0F, 90.0F}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_actor_and_health_removal_reattach(void)
{
    Fixture fixture;
    HTHEntityHandle source;
    HTHEntityHandle target;
    HTHDamageIntent intent;
    HTHDamageResolution resolution;

    CHECK(fixture_create(&fixture));
    CHECK(create_actor(&fixture, &source));
    CHECK(create_actor(&fixture, &target));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, target,
                                  (HTHHealth){50.0F, 50.0F}));
    intent = (HTHDamageIntent){source, target, 5.0F};

    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, source));
    CHECK(!hth_damage_intent_is_valid(&intent, fixture.entities,
                                      fixture.actors));
    CHECK(!hth_damage_intent_resolve(&intent, fixture.entities,
                                     fixture.actors, fixture.health,
                                     &resolution));
    CHECK(get_health(&fixture, target, (HTHHealth){50.0F, 50.0F}));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, source));
    CHECK(hth_damage_intent_is_valid(&intent, fixture.entities,
                                     fixture.actors));

    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, target));
    CHECK(!hth_damage_intent_is_valid(&intent, fixture.entities,
                                      fixture.actors));
    CHECK(!hth_damage_intent_resolve(&intent, fixture.entities,
                                     fixture.actors, fixture.health,
                                     &resolution));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, target));
    CHECK(hth_damage_intent_is_valid(&intent, fixture.entities,
                                     fixture.actors));
    CHECK(get_health(&fixture, target, (HTHHealth){50.0F, 50.0F}));

    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, target));
    CHECK(hth_damage_intent_is_valid(&intent, fixture.entities,
                                     fixture.actors));
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution_is_zero(resolution));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, target,
                                  (HTHHealth){40.0F, 50.0F}));
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.applied && resolution.damage.previous == 40.0F &&
          resolution.damage.current == 35.0F &&
          resolution.damage.applied == 5.0F &&
          !resolution.damage.became_zero);
    CHECK(get_health(&fixture, target, (HTHHealth){35.0F, 50.0F}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_lethal_no_cascade_and_source_unchanged(void)
{
    Fixture fixture;
    HTHSpatialStore *spatial = hth_spatial_store_create();
    HTHDynamicBodyStore *bodies = hth_dynamic_body_store_create();
    HTHEntityHandle source;
    HTHEntityHandle target;
    const HTHSpatialTransform transform = {{1.0F, 2.0F, 3.0F}, 0.5F};
    const HTHDynamicBody body = {{0.5F, 1.0F, 0.5F}, {1.0F, 0.0F, 2.0F}};
    HTHDamageIntent intent;
    HTHDamageResolution resolution;

    CHECK(fixture_create(&fixture));
    CHECK(spatial != NULL && bodies != NULL);
    CHECK(create_actor(&fixture, &source));
    CHECK(create_actor(&fixture, &target));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, source,
                                  (HTHHealth){60.0F, 60.0F}));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, target,
                                  (HTHHealth){20.0F, 100.0F}));
    CHECK(hth_spatial_store_attach(spatial, fixture.entities, target,
                                   &transform));
    CHECK(hth_dynamic_body_attach(bodies, fixture.entities, spatial, target,
                                  &body));
    intent = (HTHDamageIntent){source, target, 20.0F};
    CHECK(hth_damage_intent_resolve(&intent, fixture.entities,
                                    fixture.actors, fixture.health,
                                    &resolution));
    CHECK(resolution.applied && resolution.damage.became_zero);
    CHECK(get_health(&fixture, source, (HTHHealth){60.0F, 60.0F}));
    CHECK(get_health(&fixture, target, (HTHHealth){0.0F, 100.0F}));
    CHECK(hth_entity_registry_is_alive(fixture.entities, source));
    CHECK(hth_entity_registry_is_alive(fixture.entities, target));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, source));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, target));
    CHECK(hth_spatial_store_has(spatial, fixture.entities, target));
    CHECK(hth_dynamic_body_has(bodies, fixture.entities, target));
    hth_dynamic_body_store_destroy(bodies);
    hth_spatial_store_destroy(spatial);
    fixture_destroy(&fixture);
    return true;
}

static bool test_multiple_registry_store_sets(void)
{
    Fixture first;
    Fixture second;
    HTHEntityHandle first_source;
    HTHEntityHandle first_target;
    HTHEntityHandle second_source;
    HTHEntityHandle second_target;
    HTHDamageIntent first_intent;
    HTHDamageIntent second_intent;
    HTHDamageResolution resolution;

    CHECK(fixture_create(&first));
    CHECK(fixture_create(&second));
    CHECK(create_actor(&first, &first_source));
    CHECK(create_actor(&first, &first_target));
    CHECK(create_actor(&second, &second_source));
    CHECK(create_actor(&second, &second_target));
    CHECK(hth_health_store_attach(first.health, first.entities, first.actors,
                                  first_target,
                                  (HTHHealth){30.0F, 30.0F}));
    CHECK(hth_health_store_attach(second.health, second.entities,
                                  second.actors, second_target,
                                  (HTHHealth){80.0F, 80.0F}));
    first_intent = (HTHDamageIntent){first_source, first_target, 5.0F};
    second_intent = (HTHDamageIntent){second_source, second_target, 10.0F};
    CHECK(hth_damage_intent_resolve(&first_intent, first.entities,
                                    first.actors, first.health, &resolution));
    CHECK(resolution.applied && resolution.damage.current == 25.0F);
    CHECK(hth_damage_intent_resolve(&second_intent, second.entities,
                                    second.actors, second.health, &resolution));
    CHECK(resolution.applied && resolution.damage.current == 70.0F);
    CHECK(get_health(&first, first_target, (HTHHealth){25.0F, 30.0F}));
    CHECK(get_health(&second, second_target, (HTHHealth){70.0F, 80.0F}));
    fixture_destroy(&second);
    fixture_destroy(&first);
    return true;
}

int main(void)
{
    const bool passed =
        test_validation_arguments_amounts_and_purity() &&
        test_resolution_matrix_source_without_health_and_zero() &&
        test_damage_edges_self_damage_and_repeated_resolution() &&
        test_source_and_target_death_generation_reuse() &&
        test_actor_and_health_removal_reattach() &&
        test_lethal_no_cascade_and_source_unchanged() &&
        test_multiple_registry_store_sets();

    if (!passed) {
        return EXIT_FAILURE;
    }
    puts("damage intent foundation tests passed");
    return EXIT_SUCCESS;
}
