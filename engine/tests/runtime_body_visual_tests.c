#include "player_body.h"
#include "player_target_bridge.h"
#include "runtime_body_visual.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
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
    HTHSpatialStore *spatial;
    HTHDynamicBodyStore *bodies;
    HTHHealthStore *health;
} Fixture;

static const float white[4] = {1.0F, 1.0F, 1.0F, 1.0F};

static bool close_float(float left, float right)
{
    return fabsf(left - right) <= 1.0e-5F;
}

static bool vec4_matches(HTHVec4 actual, HTHVec3 expected)
{
    return close_float(actual.x, expected.x) &&
           close_float(actual.y, expected.y) &&
           close_float(actual.z, expected.z) && close_float(actual.w, 1.0F);
}

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
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

static bool attach_body(Fixture *fixture, HTHEntityHandle entity,
                        HTHSpatialTransform transform, HTHDynamicBody body)
{
    return hth_spatial_store_attach(fixture->spatial, fixture->entities,
                                    entity, &transform) &&
           hth_dynamic_body_attach(fixture->bodies, fixture->entities,
                                   fixture->spatial, entity, &body);
}

static bool test_arguments_and_not_renderable_states(void)
{
    Fixture fixture;
    HTHRendererTransientDraw draw;
    HTHEntityHandle entity;
    HTHSpatialTransform transform = {{1.0F, 2.0F, 3.0F}, 0.0F};
    HTHDynamicBody body = {{0.5F, 1.0F, 1.5F}, {0.0F, 0.0F, 0.0F}};
    float invalid_color[4] = {NAN, 1.0F, 1.0F, 1.0F};

    CHECK(fixture_create(&fixture));
    entity = hth_entity_handle_invalid();
    CHECK(hth_runtime_body_visual_build(
              NULL, fixture.spatial, fixture.bodies, entity, white, &draw) ==
          HTH_RUNTIME_BODY_VISUAL_ERROR);
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, NULL, fixture.bodies, entity, white, &draw) ==
          HTH_RUNTIME_BODY_VISUAL_ERROR);
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, NULL, entity, white, &draw) ==
          HTH_RUNTIME_BODY_VISUAL_ERROR);
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, entity, NULL,
              &draw) == HTH_RUNTIME_BODY_VISUAL_ERROR);
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, entity,
              invalid_color, &draw) == HTH_RUNTIME_BODY_VISUAL_ERROR);
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, entity, white,
              NULL) == HTH_RUNTIME_BODY_VISUAL_ERROR);
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, entity, white,
              &draw) == HTH_RUNTIME_BODY_VISUAL_NOT_RENDERABLE);
    CHECK(hth_entity_registry_create_entity(fixture.entities, &entity));
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, entity, white,
              &draw) == HTH_RUNTIME_BODY_VISUAL_NOT_RENDERABLE);
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities, entity,
                                   &transform));
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, entity, white,
              &draw) == HTH_RUNTIME_BODY_VISUAL_NOT_RENDERABLE);
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, entity, &body));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, entity));
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, entity, white,
              &draw) == HTH_RUNTIME_BODY_VISUAL_NOT_RENDERABLE);
    fixture_destroy(&fixture);
    return true;
}

static bool test_exact_transform_and_updates(void)
{
    Fixture fixture;
    HTHEntityHandle entity;
    HTHSpatialTransform transform = {{3.0F, 4.0F, 5.0F}, 0.0F};
    HTHSpatialTransform before_transform;
    HTHDynamicBody body = {{0.3F, 0.9F, 0.7F}, {1.0F, 2.0F, 3.0F}};
    HTHDynamicBody before_body;
    HTHRendererTransientDraw first;
    HTHRendererTransientDraw repeated;
    const float color[4] = {0.1F, 0.2F, 0.3F, 1.0F};
    HTHVec4 center;
    HTHVec4 maximum;
    HTHVec4 minimum;
    size_t component;

    CHECK(fixture_create(&fixture));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &entity));
    CHECK(attach_body(&fixture, entity, transform, body));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, entity,
                                &before_transform));
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities, entity,
                               &before_body));
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, entity, color,
              &first) == HTH_RUNTIME_BODY_VISUAL_READY);
    CHECK(first.primitive == HTH_GEOMETRY_PRIMITIVE_BOX);
    center = hth_mat4_transform_vec4(
        first.model, (HTHVec4){0.0F, 0.0F, 0.0F, 1.0F});
    maximum = hth_mat4_transform_vec4(
        first.model, (HTHVec4){0.5F, 0.5F, 0.5F, 1.0F});
    minimum = hth_mat4_transform_vec4(
        first.model, (HTHVec4){-0.5F, -0.5F, -0.5F, 1.0F});
    CHECK(vec4_matches(center, transform.position));
    CHECK(vec4_matches(maximum, hth_vec3(3.3F, 4.9F, 5.7F)));
    CHECK(vec4_matches(minimum, hth_vec3(2.7F, 3.1F, 4.3F)));
    for (component = 0U; component < 4U; ++component) {
        CHECK(first.base_color[component] == color[component]);
    }
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, entity, color,
              &repeated) == HTH_RUNTIME_BODY_VISUAL_READY);
    for (component = 0U; component < 16U; ++component) {
        CHECK(repeated.model.elements[component] ==
              first.model.elements[component]);
    }
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, entity,
                                &transform));
    CHECK(transform.position.x == before_transform.position.x &&
          transform.position.y == before_transform.position.y &&
          transform.position.z == before_transform.position.z &&
          transform.yaw == before_transform.yaw);
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities, entity, &body));
    CHECK(body.half_extents.x == before_body.half_extents.x &&
          body.half_extents.y == before_body.half_extents.y &&
          body.half_extents.z == before_body.half_extents.z &&
          body.velocity.x == before_body.velocity.x &&
          body.velocity.y == before_body.velocity.y &&
          body.velocity.z == before_body.velocity.z);

    transform.position = hth_vec3(-2.0F, 8.0F, 11.0F);
    transform.yaw = 1.57079632679F;
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, entity,
                                &transform));
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, entity, color,
              &repeated) == HTH_RUNTIME_BODY_VISUAL_READY);
    center = hth_mat4_transform_vec4(
        repeated.model, (HTHVec4){0.0F, 0.0F, 0.0F, 1.0F});
    maximum = hth_mat4_transform_vec4(
        repeated.model, (HTHVec4){0.0F, 0.0F, -0.5F, 1.0F});
    CHECK(vec4_matches(center, transform.position));
    CHECK(vec4_matches(maximum, hth_vec3(-1.3F, 8.0F, 11.0F)));

    CHECK(hth_dynamic_body_remove(fixture.bodies, fixture.entities, entity));
    body.half_extents = hth_vec3(1.0F, 2.0F, 3.0F);
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, entity, &body));
    transform.yaw = 0.0F;
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, entity,
                                &transform));
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, entity, color,
              &repeated) == HTH_RUNTIME_BODY_VISUAL_READY);
    maximum = hth_mat4_transform_vec4(
        repeated.model, (HTHVec4){0.5F, 0.5F, 0.5F, 1.0F});
    CHECK(vec4_matches(maximum, hth_vec3(-1.0F, 10.0F, 14.0F)));
    fixture_destroy(&fixture);
    return true;
}

static bool test_generation_store_and_proxy_isolation(void)
{
    Fixture fixture;
    HTHSpatialStore *other_spatial;
    HTHDynamicBodyStore *other_bodies;
    HTHEntityHandle stale;
    HTHEntityHandle replacement;
    HTHSpatialTransform transform = {{1.0F, 2.0F, 3.0F}, 0.0F};
    HTHDynamicBody body = {{0.5F, 1.0F, 0.5F}, {0.0F, 0.0F, 0.0F}};
    HTHRendererTransientDraw draw;
    HTHPlayerTargetBridge bridge = {0};
    HTHPlayerBody player;

    CHECK(fixture_create(&fixture));
    other_spatial = hth_spatial_store_create();
    other_bodies = hth_dynamic_body_store_create();
    CHECK(other_spatial != NULL && other_bodies != NULL);
    CHECK(hth_entity_registry_create_entity(fixture.entities, &stale));
    CHECK(attach_body(&fixture, stale, transform, body));
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, other_spatial, fixture.bodies, stale, white,
              &draw) == HTH_RUNTIME_BODY_VISUAL_NOT_RENDERABLE);
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, other_bodies, stale, white,
              &draw) == HTH_RUNTIME_BODY_VISUAL_NOT_RENDERABLE);
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &replacement));
    CHECK(replacement.index == stale.index &&
          replacement.generation != stale.generation);
    transform.position.x = 9.0F;
    CHECK(attach_body(&fixture, replacement, transform, body));
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, stale, white,
              &draw) == HTH_RUNTIME_BODY_VISUAL_NOT_RENDERABLE);
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, replacement,
              white, &draw) == HTH_RUNTIME_BODY_VISUAL_READY);

    bridge.target_entity = hth_entity_handle_invalid();
    CHECK(hth_player_body_init(&player, hth_vec3(0.0F, 0.05F, 0.0F)));
    CHECK(hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, fixture.health, &player,
        (HTHHealth){100.0F, 100.0F}));
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies,
              bridge.target_entity, white, &draw) ==
          HTH_RUNTIME_BODY_VISUAL_NOT_RENDERABLE);
    CHECK(hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, fixture.health));
    hth_dynamic_body_store_destroy(other_bodies);
    hth_spatial_store_destroy(other_spatial);
    fixture_destroy(&fixture);
    return true;
}

static bool test_large_values_and_unrepresentable_scale(void)
{
    Fixture fixture;
    HTHEntityHandle entity;
    HTHSpatialTransform transform = {{FLT_MAX, -FLT_MAX, FLT_MAX}, 0.0F};
    HTHDynamicBody body = {{1.0F, 2.0F, 3.0F}, {0.0F, 0.0F, 0.0F}};
    HTHRendererTransientDraw draw;

    CHECK(fixture_create(&fixture));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &entity));
    CHECK(attach_body(&fixture, entity, transform, body));
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, entity, white,
              &draw) == HTH_RUNTIME_BODY_VISUAL_READY);
    CHECK(hth_dynamic_body_remove(fixture.bodies, fixture.entities, entity));
    body.half_extents.x = FLT_MAX;
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, entity, &body));
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, entity, white,
              &draw) == HTH_RUNTIME_BODY_VISUAL_ERROR);
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    const struct {
        const char *name;
        bool (*run)(void);
    } tests[] = {
        {"arguments/not-renderable", test_arguments_and_not_renderable_states},
        {"exact transform/updates", test_exact_transform_and_updates},
        {"generation/store/proxy isolation",
         test_generation_store_and_proxy_isolation},
        {"large/unrepresentable values",
         test_large_values_and_unrepresentable_scale}
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
