#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include "resource.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#include <io.h>
#include <process.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

typedef struct {
    char *root;
    char *levels;
    char *directory_resource;
    char *vacant_root;
    char *binary_path;
    char *text_path;
    char *empty_path;
    char *nested_path;
} ResourceFixture;

static char *duplicate_string(const char *text)
{
    size_t length = strlen(text) + 1U;
    char *copy = malloc(length);

    assert(copy != NULL);
    memcpy(copy, text, length);
    return copy;
}

static int make_directory(const char *path)
{
#if defined(_WIN32)
    return _mkdir(path);
#else
    return mkdir(path, 0700);
#endif
}

static int remove_file(const char *path)
{
#if defined(_WIN32)
    return _unlink(path);
#else
    return unlink(path);
#endif
}

static int remove_directory(const char *path)
{
#if defined(_WIN32)
    return _rmdir(path);
#else
    return rmdir(path);
#endif
}

static int change_directory(const char *path)
{
#if defined(_WIN32)
    return _chdir(path);
#else
    return chdir(path);
#endif
}

static char *current_directory(void)
{
#if defined(_WIN32)
    return _getcwd(NULL, 0);
#else
    return getcwd(NULL, 0U);
#endif
}

static char *create_temporary_root(void)
{
#if defined(_WIN32)
    char *working_directory = current_directory();
    char *path;
    size_t capacity;
    unsigned int attempt;

    assert(working_directory != NULL);
    capacity = strlen(working_directory) + 64U;
    path = malloc(capacity);
    assert(path != NULL);
    for (attempt = 0U; attempt < 1000U; ++attempt) {
        int written = snprintf(path, capacity,
                               "%s/hth-resource-tests-%u-%u",
                               working_directory, (unsigned int)_getpid(),
                               attempt);

        assert(written > 0 && (size_t)written < capacity);
        if (make_directory(path) == 0) {
            free(working_directory);
            return path;
        }
        assert(errno == EEXIST);
    }
    assert(false);
    return NULL;
#else
    char *path = duplicate_string("/tmp/hth-resource-tests-XXXXXX");

    assert(mkdtemp(path) == path);
    return path;
#endif
}

static char *join_path(const char *left, const char *right)
{
    size_t left_length = strlen(left);
    size_t right_length = strlen(right);
    char *path;

    assert(left_length <= SIZE_MAX - right_length - 2U);
    path = malloc(left_length + right_length + 2U);
    assert(path != NULL);
    memcpy(path, left, left_length);
    path[left_length] = '/';
    memcpy(path + left_length + 1U, right, right_length + 1U);
    return path;
}

static void write_file(const char *path, const unsigned char *bytes,
                       size_t size)
{
    FILE *file = fopen(path, "wb");

    assert(file != NULL);
    if (size > 0U) {
        assert(bytes != NULL);
        assert(fwrite(bytes, 1U, size, file) == size);
    }
    assert(fclose(file) == 0);
}

static ResourceFixture fixture_create(void)
{
    static const unsigned char binary[] = {
        0x00U, 0xFFU, 0x41U, 0x00U, 0x42U, 0x7FU
    };
    static const unsigned char text[] = {'h', 'e', 'l', 'l', 'o'};
    ResourceFixture fixture = {0};

    fixture.root = create_temporary_root();
    fixture.levels = join_path(fixture.root, "levels");
    fixture.directory_resource = join_path(fixture.root, "directory");
    fixture.vacant_root = join_path(fixture.root, "vacant");
    assert(make_directory(fixture.levels) == 0);
    assert(make_directory(fixture.directory_resource) == 0);
    assert(make_directory(fixture.vacant_root) == 0);
    fixture.binary_path = join_path(fixture.root, "binary.bin");
    fixture.text_path = join_path(fixture.root, "hello.txt");
    fixture.empty_path = join_path(fixture.root, "empty.bin");
    fixture.nested_path = join_path(fixture.levels, "test.bin");
    write_file(fixture.binary_path, binary, sizeof(binary));
    write_file(fixture.text_path, text, sizeof(text));
    write_file(fixture.empty_path, NULL, 0U);
    write_file(fixture.nested_path, text, sizeof(text));
    return fixture;
}

static void fixture_destroy(ResourceFixture *fixture)
{
    assert(remove_file(fixture->binary_path) == 0);
    assert(remove_file(fixture->text_path) == 0);
    assert(remove_file(fixture->empty_path) == 0);
    assert(remove_file(fixture->nested_path) == 0);
    assert(remove_directory(fixture->levels) == 0);
    assert(remove_directory(fixture->directory_resource) == 0);
    assert(remove_directory(fixture->vacant_root) == 0);
    assert(remove_directory(fixture->root) == 0);
    free(fixture->nested_path);
    free(fixture->empty_path);
    free(fixture->text_path);
    free(fixture->binary_path);
    free(fixture->vacant_root);
    free(fixture->directory_resource);
    free(fixture->levels);
    free(fixture->root);
    *fixture = (ResourceFixture){0};
}

static void test_resource_ids(void)
{
    static const char *const valid[] = {
        "foo", "foo.bar", "foo/bar", "levels/bootstrap.hthlevel",
        "textures/world/stone_01.png", "audio/player/land-heavy.wav",
        "models/enemies/wraith.v1.glb", "a/b-c_d.123", "0", "a0/b1",
        "foo/bar.v1.data", "a", "a-b_c.1"
    };
    static const char *const invalid[] = {
        "", "/foo", "foo/", "foo//bar", "foo/./bar", "foo/../bar",
        "../foo", "./foo", ".", "..", "foo\\bar", "Foo/bar", "FOO",
        "foo bar", "foo/.hidden", ".hidden", "foo:bar", "foo*bar",
        "foo?bar", "foo\"bar", "foo<bar", "foo>bar", "foo|bar",
        "foo\tbar", "caf\xC3\xA9"
    };
    size_t index;

    assert(!hth_resource_id_is_valid(NULL));
    for (index = 0U; index < sizeof(valid) / sizeof(valid[0]); ++index) {
        assert(hth_resource_id_is_valid(valid[index]));
    }
    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        assert(!hth_resource_id_is_valid(invalid[index]));
    }
}

static void test_root_validation(const ResourceFixture *fixture)
{
    HTHResourceConfig config = {0};
    HTHResourceSystem *resources;
    char *missing = join_path(fixture->root, "missing-root");
    char *trailing = join_path(fixture->vacant_root, "");

    assert(hth_resource_system_create(NULL) == NULL);
    assert(hth_resource_system_create(&config) == NULL);
    config.root_path = "";
    assert(hth_resource_system_create(&config) == NULL);
    config.root_path = missing;
    assert(hth_resource_system_create(&config) == NULL);
    config.root_path = fixture->text_path;
    assert(hth_resource_system_create(&config) == NULL);
    config.root_path = fixture->vacant_root;
    resources = hth_resource_system_create(&config);
    assert(resources != NULL);
    hth_resource_system_destroy(resources);
    config.root_path = trailing;
    resources = hth_resource_system_create(&config);
    assert(resources != NULL);
    hth_resource_system_destroy(resources);
    hth_resource_system_destroy(NULL);
    free(trailing);
    free(missing);
}

static void test_resolution(const ResourceFixture *fixture)
{
    HTHResourceConfig config = {fixture->root};
    HTHResourceSystem *resources = hth_resource_system_create(&config);
    HTHResourceSystem *trailing_resources;
    char resource_id[] = "levels/test.bin";
    char *expected = join_path(fixture->root, resource_id);
    char *trailing_root = join_path(fixture->root, "");
    char *resolved = NULL;
    char *occupied = malloc(1U);
    char *long_id;
    size_t index;

    assert(resources != NULL && occupied != NULL);
    assert(hth_resource_resolve_path(resources, resource_id, &resolved));
    assert(strcmp(resolved, expected) == 0);
    assert(strcmp(resource_id, "levels/test.bin") == 0);
    free(resolved);
    resolved = NULL;
    config.root_path = trailing_root;
    trailing_resources = hth_resource_system_create(&config);
    assert(trailing_resources != NULL);
    assert(hth_resource_resolve_path(
        trailing_resources, resource_id, &resolved));
    assert(strcmp(resolved, expected) == 0);
    free(resolved);
    hth_resource_system_destroy(trailing_resources);
    resolved = occupied;
    assert(!hth_resource_resolve_path(resources, resource_id, &resolved));
    assert(resolved == occupied);
    free(occupied);
    resolved = NULL;
    assert(!hth_resource_resolve_path(resources, "../secret", &resolved));
    assert(resolved == NULL);

    long_id = malloc(1025U);
    assert(long_id != NULL);
    for (index = 0U; index < 1024U; ++index) {
        long_id[index] = 'a';
    }
    long_id[1024] = '\0';
    assert(hth_resource_resolve_path(resources, long_id, &resolved));
    assert(strlen(resolved) == strlen(fixture->root) + 1U + 1024U);
    free(resolved);
    free(long_id);
    free(trailing_root);
    free(expected);
    hth_resource_system_destroy(resources);
}

static void test_relative_root_stability_and_copy(
    const ResourceFixture *fixture)
{
    const char *separator = strrchr(fixture->root, '/');
    HTHResourceConfig config;
    HTHResourceSystem *resources;
    HTHResourceData data = {0};
    char *original_cwd = current_directory();
    char *parent;
    char *mutable_root;
    size_t parent_length;

    assert(separator != NULL && original_cwd != NULL);
    parent_length = (size_t)(separator - fixture->root);
    parent = malloc(parent_length + 1U);
    mutable_root = duplicate_string(separator + 1);
    assert(parent != NULL && mutable_root != NULL);
    memcpy(parent, fixture->root, parent_length);
    parent[parent_length] = '\0';
    assert(change_directory(parent) == 0);
    config.root_path = mutable_root;
    resources = hth_resource_system_create(&config);
    assert(resources != NULL);
    assert(!hth_resource_load(resources, "hello.txt", NULL));
    memset(mutable_root, 'x', strlen(mutable_root));
    assert(change_directory(original_cwd) == 0);
    assert(hth_resource_load(resources, "hello.txt", &data));
    assert(data.size == 5U && memcmp(data.data, "hello", 5U) == 0);
    hth_resource_data_release(&data);
    hth_resource_system_destroy(resources);
    free(mutable_root);
    free(parent);
    free(original_cwd);
}

static void test_loading_and_ownership(const ResourceFixture *fixture)
{
    static const unsigned char expected_binary[] = {
        0x00U, 0xFFU, 0x41U, 0x00U, 0x42U, 0x7FU
    };
    HTHResourceConfig config = {fixture->root};
    HTHResourceSystem *resources = hth_resource_system_create(&config);
    HTHResourceData first = {0};
    HTHResourceData second = {0};
    HTHResourceData empty = {0};
    HTHResourceData missing = {0};

    assert(resources != NULL);
    assert(hth_resource_load(resources, "binary.bin", &first));
    assert(first.size == sizeof(expected_binary));
    assert(memcmp(first.data, expected_binary, sizeof(expected_binary)) == 0);
    assert(hth_resource_load(resources, "binary.bin", &second));
    assert(second.size == first.size);
    assert(second.data != first.data);
    first.data[0] = 0x33U;
    assert(second.data[0] == expected_binary[0]);
    assert(hth_resource_load(resources, "empty.bin", &empty));
    assert(empty.data == NULL && empty.size == 0U);
    hth_resource_data_release(&empty);
    hth_resource_data_release(&empty);
    assert(!hth_resource_load(resources, "missing.bin", &missing));
    assert(missing.data == NULL && missing.size == 0U);
    assert(!hth_resource_load(resources, "directory", &missing));
    assert(!hth_resource_load(resources, "../secret", &missing));
    hth_resource_system_destroy(resources);
    assert(second.size == sizeof(expected_binary));
    assert(memcmp(second.data, expected_binary, sizeof(expected_binary)) == 0);
    hth_resource_data_release(&first);
    hth_resource_data_release(&second);
    hth_resource_data_release(NULL);
}

static void test_output_precondition_and_reuse(const ResourceFixture *fixture)
{
    HTHResourceConfig config = {fixture->root};
    HTHResourceSystem *resources = hth_resource_system_create(&config);
    HTHResourceData owned;
    HTHResourceData stale_size = {NULL, 1U};
    HTHResourceData reusable = {0};
    unsigned char *caller_byte = malloc(1U);

    assert(resources != NULL && caller_byte != NULL);
    caller_byte[0] = 0xA5U;
    owned.data = caller_byte;
    owned.size = 1U;
    assert(!hth_resource_load(resources, "hello.txt", &owned));
    assert(owned.data == caller_byte && owned.size == 1U);
    assert(owned.data[0] == 0xA5U);
    assert(!hth_resource_load(resources, "hello.txt", &stale_size));
    assert(stale_size.data == NULL && stale_size.size == 1U);
    free(caller_byte);
    owned.data = NULL;
    owned.size = 0U;

    assert(hth_resource_load(resources, "hello.txt", &reusable));
    assert(reusable.size == 5U && memcmp(reusable.data, "hello", 5U) == 0);
    hth_resource_data_release(&reusable);
    assert(!hth_resource_load(resources, "missing.bin", &reusable));
    assert(reusable.data == NULL && reusable.size == 0U);
    hth_resource_system_destroy(resources);
}

int main(void)
{
    ResourceFixture fixture = fixture_create();

    test_resource_ids();
    test_root_validation(&fixture);
    test_resolution(&fixture);
    test_relative_root_stability_and_copy(&fixture);
    test_loading_and_ownership(&fixture);
    test_output_precondition_and_reuse(&fixture);
    fixture_destroy(&fixture);
    return 0;
}
