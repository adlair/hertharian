#ifndef HTH_RESOURCE_H
#define HTH_RESOURCE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct HTHResourceSystem HTHResourceSystem;

typedef struct {
    const char *root_path;
} HTHResourceConfig;

typedef struct {
    unsigned char *data;
    size_t size;
} HTHResourceData;

HTHResourceSystem *hth_resource_system_create(
    const HTHResourceConfig *config);
void hth_resource_system_destroy(HTHResourceSystem *resources);

bool hth_resource_id_is_valid(const char *resource_id);

/* The caller owns a successful non-NULL path and releases it with free(). */
bool hth_resource_resolve_path(const HTHResourceSystem *resources,
                               const char *resource_id, char **out_path);

bool hth_resource_load(const HTHResourceSystem *resources,
                       const char *resource_id, HTHResourceData *out_data);
void hth_resource_data_release(HTHResourceData *data);

#endif
