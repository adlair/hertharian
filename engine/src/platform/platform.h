#ifndef HTH_PLATFORM_H
#define HTH_PLATFORM_H

#include "event.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct HTHPlatform HTHPlatform;
typedef struct HTHPlatformGraphicsContext HTHPlatformGraphicsContext;
typedef void (*HTHGraphicsProcedure)(void);

typedef struct {
    const char *window_title;
    uint32_t window_width;
    uint32_t window_height;
    bool headless;
    bool graphics_enabled;
} HTHPlatformConfig;

bool hth_platform_init(HTHPlatform **platform, const HTHPlatformConfig *config);
void hth_platform_shutdown(HTHPlatform *platform);
bool hth_platform_poll_event(HTHPlatform *platform, HTHPlatformEvent *event);
uint64_t hth_platform_time_counter(void);
uint64_t hth_platform_time_frequency(void);
void hth_platform_sleep_ns(uint64_t nanoseconds);
HTHPlatformGraphicsContext *hth_platform_graphics_create_context(
    HTHPlatform *platform);
void hth_platform_graphics_destroy_context(
    HTHPlatformGraphicsContext *context);
bool hth_platform_graphics_make_current(
    HTHPlatform *platform, HTHPlatformGraphicsContext *context);
bool hth_platform_graphics_swap(HTHPlatform *platform);
bool hth_platform_graphics_set_swap_interval(int interval);
bool hth_platform_graphics_context_info(int *major, int *minor,
                                        bool *core_profile, int *double_buffer,
                                        int *depth_bits, int *stencil_bits);
bool hth_platform_framebuffer_size(HTHPlatform *platform,
                                   uint32_t *width, uint32_t *height);
HTHGraphicsProcedure hth_platform_graphics_get_proc_address(
    HTHPlatform *platform, const char *name);

#endif
