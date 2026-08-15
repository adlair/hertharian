#ifndef HTH_LEVEL_H
#define HTH_LEVEL_H

#include "world.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HTH_LEVEL_FORMAT_VERSION 1U
#define HTH_LEVEL_ERROR_MESSAGE_CAPACITY 160U

typedef struct {
    HTHWorldStaticObject object;
    size_t source_line;
    size_t source_column;
} HTHLevelStaticObjectDescription;

typedef struct {
    unsigned int format_version;
    HTHWorldSpawn default_spawn;
    size_t spawn_line;
    size_t spawn_column;
    HTHLevelStaticObjectDescription *objects;
    size_t object_count;
    size_t object_capacity;
    bool has_default_spawn;
} HTHLevelDescription;

typedef struct {
    size_t line;
    size_t column;
    char message[HTH_LEVEL_ERROR_MESSAGE_CAPACITY];
} HTHLevelError;

bool hth_level_parse(const unsigned char *data, size_t size,
                     HTHLevelDescription *out_description,
                     HTHLevelError *out_error);
void hth_level_description_destroy(HTHLevelDescription *description);

bool hth_level_build_world(const HTHLevelDescription *description,
                           HTHWorld *out_world, HTHLevelError *out_error);

#endif
