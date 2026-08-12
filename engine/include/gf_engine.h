#ifndef GF_ENGINE_H
#define GF_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint64_t frame_limit;
} GFEngineConfig;

typedef struct {
    uint64_t frame_number;
    uint64_t frame_limit;
    bool initialized;
    bool running;
} GFEngine;

const char *gf_engine_version(void);
bool gf_engine_init(GFEngine *engine, const GFEngineConfig *config);
void gf_engine_run(GFEngine *engine);
void gf_engine_frame(GFEngine *engine);
void gf_engine_shutdown(GFEngine *engine);

#endif
