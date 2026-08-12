#ifndef HTH_ENGINE_H
#define HTH_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint64_t frame_limit;
} HTHEngineConfig;

typedef struct {
    uint64_t frame_number;
    uint64_t frame_limit;
    bool initialized;
    bool running;
} HTHEngine;

const char *hth_engine_version(void);
bool hth_engine_init(HTHEngine *engine, const HTHEngineConfig *config);
void hth_engine_run(HTHEngine *engine);
void hth_engine_frame(HTHEngine *engine);
void hth_engine_shutdown(HTHEngine *engine);

#endif
