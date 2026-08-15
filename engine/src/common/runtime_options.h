#ifndef HTH_RUNTIME_OPTIONS_H
#define HTH_RUNTIME_OPTIONS_H

#include "hth_engine.h"

#include <stdbool.h>

typedef enum {
    HTH_RUNTIME_OPTIONS_ERROR_NONE = 0,
    HTH_RUNTIME_OPTIONS_ERROR_INVALID_ARGUMENTS,
    HTH_RUNTIME_OPTIONS_ERROR_MISSING_LEVEL_VALUE,
    HTH_RUNTIME_OPTIONS_ERROR_DUPLICATE_LEVEL,
    HTH_RUNTIME_OPTIONS_ERROR_INVALID_LEVEL_ID
} HTHRuntimeOptionsErrorCode;

typedef struct {
    HTHRuntimeOptionsErrorCode code;
    const char *argument;
} HTHRuntimeOptionsError;

typedef struct {
    HTHEngineConfig engine;
    const char *level_id;
} HTHRuntimeOptions;

const char *hth_runtime_options_default_level_id(void);
bool hth_runtime_options_parse(int argc, char *const argv[],
                               HTHRuntimeOptions *out_options,
                               HTHRuntimeOptionsError *out_error);

#endif
