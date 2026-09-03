#include "player_death.h"

bool hth_player_death_is_dead(const HTHEntityRegistry *entities,
                              const HTHActorStore *actors,
                              const HTHHealthStore *health,
                              HTHEntityHandle player,
                              bool *out_dead)
{
    HTHHealth player_health;

    if (out_dead != NULL) {
        *out_dead = false;
    }
    if (entities == NULL || actors == NULL || health == NULL ||
        out_dead == NULL ||
        !hth_health_store_get(health, entities, actors, player,
                              &player_health) ||
        !hth_health_is_valid(player_health)) {
        return false;
    }
    *out_dead = player_health.current == 0.0F;
    return true;
}
