#ifndef HTH_PLAYER_DEATH_H
#define HTH_PLAYER_DEATH_H

#include "actor.h"
#include "entity.h"
#include "health.h"

#include <stdbool.h>

bool hth_player_death_is_dead(const HTHEntityRegistry *entities,
                              const HTHActorStore *actors,
                              const HTHHealthStore *health,
                              HTHEntityHandle player,
                              bool *out_dead);

#endif
