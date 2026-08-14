#if !defined(_WIN32)
#define _XOPEN_SOURCE 700
#endif

#include "resource.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#endif

struct HTHResourceSystem {
    char *root_path;
};

static bool resource_character_is_valid(unsigned char character)
{
    return (character >= (unsigned char)'a' &&
            character <= (unsigned char)'z') ||
           (character >= (unsigned char)'0' &&
            character <= (unsigned char)'9') ||
           character == (unsigned char)'_' ||
           character == (unsigned char)'-' ||
           character == (unsigned char)'.';
}

bool hth_resource_id_is_valid(const char *resource_id)
{
    const unsigned char *cursor;
    bool segment_start = true;

    if (resource_id == NULL || resource_id[0] == '\0') {
        return false;
    }
    cursor = (const unsigned char *)resource_id;
    while (*cursor != (unsigned char)'\0') {
        if (*cursor == (unsigned char)'/') {
            if (segment_start) {
                return false;
            }
            segment_start = true;
        } else {
            if (!resource_character_is_valid(*cursor) ||
                (segment_start && *cursor == (unsigned char)'.')) {
                return false;
            }
            segment_start = false;
        }
        cursor++;
    }
    return !segment_start;
}

static bool path_is_directory(const char *path)
{
#if defined(_WIN32)
    struct _stat64 status;

    return _stat64(path, &status) == 0 &&
           (status.st_mode & _S_IFMT) == _S_IFDIR;
#else
    struct stat status;

    return stat(path, &status) == 0 && S_ISDIR(status.st_mode);
#endif
}

static char *stable_root_path(const char *path)
{
    char *resolved;

    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
#if defined(_WIN32)
    resolved = _fullpath(NULL, path, 0U);
#else
    resolved = realpath(path, NULL);
#endif
    if (resolved == NULL || !path_is_directory(resolved)) {
        free(resolved);
        return NULL;
    }
    return resolved;
}

HTHResourceSystem *hth_resource_system_create(
    const HTHResourceConfig *config)
{
    HTHResourceSystem *resources;

    if (config == NULL) {
        return NULL;
    }
    resources = calloc(1, sizeof(*resources));
    if (resources == NULL) {
        return NULL;
    }
    resources->root_path = stable_root_path(config->root_path);
    if (resources->root_path == NULL) {
        free(resources);
        return NULL;
    }
    return resources;
}

void hth_resource_system_destroy(HTHResourceSystem *resources)
{
    if (resources == NULL) {
        return;
    }
    free(resources->root_path);
    free(resources);
}

static bool path_ends_with_separator(const char *path, size_t length)
{
    return length > 0U &&
           (path[length - 1U] == '/' || path[length - 1U] == '\\');
}

bool hth_resource_resolve_path(const HTHResourceSystem *resources,
                               const char *resource_id, char **out_path)
{
    char *path;
    size_t id_length;
    size_t path_length;
    size_t root_length;
    size_t separator_length;
    size_t total_length;

    if (resources == NULL || resources->root_path == NULL ||
        out_path == NULL || *out_path != NULL ||
        !hth_resource_id_is_valid(resource_id)) {
        return false;
    }
    root_length = strlen(resources->root_path);
    id_length = strlen(resource_id);
    separator_length = path_ends_with_separator(resources->root_path,
                                                 root_length) ? 0U : 1U;
    if (root_length > SIZE_MAX - id_length) {
        return false;
    }
    total_length = root_length + id_length;
    if (separator_length > SIZE_MAX - total_length) {
        return false;
    }
    total_length += separator_length;
    if (total_length == SIZE_MAX) {
        return false;
    }
    total_length++;

    path = malloc(total_length);
    if (path == NULL) {
        return false;
    }
    memcpy(path, resources->root_path, root_length);
    path_length = root_length;
    if (separator_length != 0U) {
        path[path_length++] = '/';
    }
    memcpy(path + path_length, resource_id, id_length);
    path[path_length + id_length] = '\0';
    *out_path = path;
    return true;
}

static bool regular_file_size(const char *path, size_t *out_size)
{
#if defined(_WIN32)
    struct _stat64 status;

    if (_stat64(path, &status) != 0 ||
        (status.st_mode & _S_IFMT) != _S_IFREG || status.st_size < 0 ||
        (uint64_t)status.st_size > (uint64_t)SIZE_MAX) {
        return false;
    }
    *out_size = (size_t)status.st_size;
#else
    struct stat status;

    if (stat(path, &status) != 0 || !S_ISREG(status.st_mode) ||
        status.st_size < 0 || (uintmax_t)status.st_size > (uintmax_t)SIZE_MAX) {
        return false;
    }
    *out_size = (size_t)status.st_size;
#endif
    return true;
}

bool hth_resource_load(const HTHResourceSystem *resources,
                       const char *resource_id, HTHResourceData *out_data)
{
    unsigned char *bytes = NULL;
    char *path = NULL;
    FILE *file = NULL;
    size_t file_size;
    bool success = false;

    if (out_data == NULL || out_data->data != NULL || out_data->size != 0U ||
        !hth_resource_resolve_path(resources, resource_id, &path) ||
        !regular_file_size(path, &file_size)) {
        free(path);
        return false;
    }
    file = fopen(path, "rb");
    free(path);
    if (file == NULL) {
        return false;
    }
    if (file_size > 0U) {
        bytes = malloc(file_size);
        if (bytes == NULL || fread(bytes, 1U, file_size, file) != file_size) {
            goto cleanup;
        }
    }
    {
        bool exact_end = fgetc(file) == EOF && ferror(file) == 0;
        int close_result = fclose(file);

        file = NULL;
        if (!exact_end || close_result != 0) {
            goto cleanup;
        }
    }
    out_data->data = bytes;
    out_data->size = file_size;
    bytes = NULL;
    success = true;

cleanup:
    if (file != NULL) {
        (void)fclose(file);
    }
    free(bytes);
    return success;
}

void hth_resource_data_release(HTHResourceData *data)
{
    if (data == NULL) {
        return;
    }
    free(data->data);
    data->data = NULL;
    data->size = 0U;
}
