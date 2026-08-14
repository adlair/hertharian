#ifndef HTH_ENGINE_H
#define HTH_ENGINE_H

#include "hth_camera.h"
#include "hth_input.h"
#include "hth_timing.h"

#include <stdbool.h>
#include <stdint.h>

struct HTHPlatform;
struct HTHRenderer;
struct HTHFPSCameraController;
struct HTHEnginePhysicalState;
struct HTHEngineViewState;
struct HTHEngineWorldState;
struct HTHEngineStorageState;

typedef struct {
    uint64_t frame_limit;
    const char *window_title;
    uint32_t window_width;
    uint32_t window_height;
    uint32_t target_fps;
    bool headless;
    bool debug_fps_input;
} HTHEngineConfig;

typedef struct {
    uint64_t frame_limit;
    uint32_t window_width;
    uint32_t window_height;
    bool initialized;
    bool running;
    bool headless;
    bool debug_fps_input;
    struct HTHPlatform *platform;
    struct HTHRenderer *renderer;
    struct HTHFPSCameraController *camera_controller;
    struct HTHEnginePhysicalState *physical_state;
    struct HTHEngineViewState *view_state;
    struct HTHEngineWorldState *world_state;
    struct HTHEngineStorageState *storage_state;
    HTHCamera camera;
    HTHInput *input;
    HTHTiming *timing;
} HTHEngine;

const char *hth_engine_version(void);
bool hth_engine_init(HTHEngine *engine, const HTHEngineConfig *config);
void hth_engine_run(HTHEngine *engine);
void hth_engine_frame(HTHEngine *engine);
void hth_engine_shutdown(HTHEngine *engine);
const HTHInput *hth_engine_input(const HTHEngine *engine);
const HTHTiming *hth_engine_timing(const HTHEngine *engine);

#endif
