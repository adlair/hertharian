#ifndef HTH_PLATFORM_H
#define HTH_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct HTHPlatform HTHPlatform;

typedef struct {
    const char *window_title;
    uint32_t window_width;
    uint32_t window_height;
} HTHPlatformConfig;

bool hth_platform_init(HTHPlatform **platform, const HTHPlatformConfig *config);
void hth_platform_shutdown(HTHPlatform *platform);
bool hth_platform_pump_events(HTHPlatform *platform);
uint64_t hth_platform_time_counter(void);
uint64_t hth_platform_time_frequency(void);
void hth_platform_sleep_ms(uint32_t milliseconds);

#endif
