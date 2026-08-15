#ifndef HTH_ENGINE_INTERNAL_H
#define HTH_ENGINE_INTERNAL_H

#include "hth_engine.h"

#include <stdbool.h>

bool hth_engine_init_with_level_id(HTHEngine *engine,
                                   const HTHEngineConfig *config,
                                   const char *level_id);

#endif
