#ifndef HTH_ENGINE_H
#define HTH_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

struct HTHPlatform;

typedef struct {
    uint64_t frame_limit;
    const char *window_title;
    uint32_t window_width;
    uint32_t window_height;
} HTHEngineConfig;

typedef struct {
    uint64_t frame_number;
    uint64_t frame_limit;
    bool initialized;
    bool running;
    struct HTHPlatform *platform;
} HTHEngine;

const char *hth_engine_version(void);
bool hth_engine_init(HTHEngine *engine, const HTHEngineConfig *config);
void hth_engine_run(HTHEngine *engine);
void hth_engine_frame(HTHEngine *engine);
void hth_engine_shutdown(HTHEngine *engine);

#endif
