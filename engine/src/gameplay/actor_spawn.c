#include "actor_spawn.h"

#include <math.h>

static bool vector_is_finite(HTHVec3 vector)
{
    return isfinite(vector.x) && isfinite(vector.y) && isfinite(vector.z);
}

static bool transform_is_valid(HTHSpatialTransform transform)
{
    return vector_is_finite(transform.position) && isfinite(transform.yaw);
}

static bool body_is_valid(HTHDynamicBody body)
{
    return vector_is_finite(body.half_extents) &&
           body.half_extents.x > 0.0F && body.half_extents.y > 0.0F &&
           body.half_extents.z > 0.0F && vector_is_finite(body.velocity);
}

static bool spec_is_valid(const HTHActorSpawnSpec *spec)
{
    return spec != NULL && (!spec->has_body || spec->has_spatial) &&
           (!spec->has_spatial || transform_is_valid(spec->transform)) &&
           (!spec->has_body || body_is_valid(spec->body)) &&
           (!spec->has_health || hth_health_is_valid(spec->health));
}

static void rollback_spawn(HTHEntityRegistry *entities,
                           HTHActorStore *actors, HTHSpatialStore *spatial,
                           HTHDynamicBodyStore *bodies,
                           HTHHealthStore *health, HTHEntityHandle entity,
                           bool actor_attached, bool spatial_attached,
                           bool body_attached, bool health_attached)
{
    if (health_attached) {
        (void)hth_health_store_remove(health, entities, actors, entity);
    }
    if (body_attached) {
        (void)hth_dynamic_body_remove(bodies, entities, entity);
    }
    if (spatial_attached) {
        (void)hth_spatial_store_remove(spatial, entities, entity);
    }
    if (actor_attached) {
        (void)hth_actor_store_remove(actors, entities, entity);
    }
    (void)hth_entity_registry_destroy_entity(entities, entity);
}

bool hth_actor_spawn(HTHEntityRegistry *entities, HTHActorStore *actors,
                     HTHSpatialStore *spatial, HTHDynamicBodyStore *bodies,
                     HTHHealthStore *health, const HTHActorSpawnSpec *spec,
                     HTHEntityHandle *out_entity)
{
    HTHEntityHandle entity;
    bool actor_attached = false;
    bool spatial_attached = false;
    bool body_attached = false;
    bool health_attached = false;

    if (out_entity != NULL) {
        *out_entity = hth_entity_handle_invalid();
    }
    if (entities == NULL || actors == NULL || spatial == NULL ||
        bodies == NULL || health == NULL || out_entity == NULL ||
        !spec_is_valid(spec)) {
        return false;
    }
    if (!hth_entity_registry_create_entity(entities, &entity)) {
        return false;
    }
    actor_attached = hth_actor_store_attach(actors, entities, entity);
    if (actor_attached && spec->has_spatial) {
        spatial_attached = hth_spatial_store_attach(
            spatial, entities, entity, &spec->transform);
    }
    if (actor_attached && (!spec->has_spatial || spatial_attached) &&
        spec->has_body) {
        body_attached = hth_dynamic_body_attach(
            bodies, entities, spatial, entity, &spec->body);
    }
    if (actor_attached && (!spec->has_spatial || spatial_attached) &&
        (!spec->has_body || body_attached) && spec->has_health) {
        health_attached = hth_health_store_attach(
            health, entities, actors, entity, spec->health);
    }
    if (!actor_attached || (spec->has_spatial && !spatial_attached) ||
        (spec->has_body && !body_attached) ||
        (spec->has_health && !health_attached)) {
        rollback_spawn(entities, actors, spatial, bodies, health, entity,
                       actor_attached, spatial_attached, body_attached,
                       health_attached);
        return false;
    }
    *out_entity = entity;
    return true;
}

bool hth_actor_despawn(HTHEntityRegistry *entities, HTHActorStore *actors,
                       HTHSpatialStore *spatial,
                       HTHDynamicBodyStore *bodies, HTHHealthStore *health,
                       HTHEntityHandle entity)
{
    if (entities == NULL || actors == NULL || spatial == NULL ||
        bodies == NULL || health == NULL ||
        !hth_actor_store_has(actors, entities, entity)) {
        return false;
    }
    if (hth_health_store_has(health, entities, actors, entity) &&
        !hth_health_store_remove(health, entities, actors, entity)) {
        return false;
    }
    if (hth_dynamic_body_has(bodies, entities, entity) &&
        !hth_dynamic_body_remove(bodies, entities, entity)) {
        return false;
    }
    if (hth_spatial_store_has(spatial, entities, entity) &&
        !hth_spatial_store_remove(spatial, entities, entity)) {
        return false;
    }
    if (!hth_actor_store_remove(actors, entities, entity)) {
        return false;
    }
    return hth_entity_registry_destroy_entity(entities, entity);
}
