#if !defined(_WIN32)
#define _GNU_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include "level.h"

#include <errno.h>
#include <locale.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
typedef _locale_t HTHNumericLocale;
#else
typedef locale_t HTHNumericLocale;
#endif

typedef struct {
    const unsigned char *data;
    size_t size;
    size_t cursor;
    size_t line;
    size_t column;
} HTHLevelScanner;

typedef struct {
    const unsigned char *data;
    size_t length;
    size_t line;
    size_t column;
} HTHLevelToken;

static void clear_error(HTHLevelError *error)
{
    if (error != NULL) {
        memset(error, 0, sizeof(*error));
    }
}

static bool fail_at(HTHLevelError *error, size_t line, size_t column,
                    const char *message)
{
    if (error != NULL) {
        error->line = line;
        error->column = column;
        (void)snprintf(error->message, sizeof(error->message), "%s", message);
    }
    return false;
}

static bool description_is_empty(const HTHLevelDescription *description)
{
    return description != NULL && description->format_version == 0U &&
           !description->has_default_spawn && description->objects == NULL &&
           description->object_count == 0U &&
           description->object_capacity == 0U;
}

void hth_level_description_destroy(HTHLevelDescription *description)
{
    if (description == NULL) {
        return;
    }
    free(description->objects);
    memset(description, 0, sizeof(*description));
}

static bool token_equals(HTHLevelToken token, const char *text)
{
    size_t length = strlen(text);

    return token.length == length &&
           memcmp(token.data, text, length) == 0;
}

static void scanner_advance_newline(HTHLevelScanner *scanner)
{
    if (scanner->data[scanner->cursor] == (unsigned char)'\r') {
        scanner->cursor++;
        if (scanner->cursor < scanner->size &&
            scanner->data[scanner->cursor] == (unsigned char)'\n') {
            scanner->cursor++;
        }
    } else {
        scanner->cursor++;
    }
    scanner->line++;
    scanner->column = 1U;
}

static void scanner_skip_whitespace_and_comments(HTHLevelScanner *scanner)
{
    while (scanner->cursor < scanner->size) {
        unsigned char character = scanner->data[scanner->cursor];

        if (character == (unsigned char)' ' ||
            character == (unsigned char)'\t') {
            scanner->cursor++;
            scanner->column++;
        } else if (character == (unsigned char)'\n' ||
                   character == (unsigned char)'\r') {
            scanner_advance_newline(scanner);
        } else if (character == (unsigned char)'#') {
            while (scanner->cursor < scanner->size &&
                   scanner->data[scanner->cursor] != (unsigned char)'\n' &&
                   scanner->data[scanner->cursor] != (unsigned char)'\r') {
                scanner->cursor++;
                scanner->column++;
            }
        } else {
            break;
        }
    }
}

static bool scanner_read_token(HTHLevelScanner *scanner,
                               HTHLevelToken *out_token)
{
    size_t start;

    scanner_skip_whitespace_and_comments(scanner);
    if (scanner->cursor == scanner->size) {
        return false;
    }
    start = scanner->cursor;
    out_token->data = scanner->data + start;
    out_token->line = scanner->line;
    out_token->column = scanner->column;
    while (scanner->cursor < scanner->size) {
        unsigned char character = scanner->data[scanner->cursor];

        if (character == (unsigned char)' ' ||
            character == (unsigned char)'\t' ||
            character == (unsigned char)'\n' ||
            character == (unsigned char)'\r' ||
            character == (unsigned char)'#') {
            break;
        }
        scanner->cursor++;
        scanner->column++;
    }
    out_token->length = scanner->cursor - start;
    return true;
}

static bool expect_token(HTHLevelScanner *scanner, const char *expected,
                         const char *message, HTHLevelError *error)
{
    HTHLevelToken token;

    if (!scanner_read_token(scanner, &token)) {
        return fail_at(error, scanner->line, scanner->column, message);
    }
    if (!token_equals(token, expected)) {
        return fail_at(error, token.line, token.column, message);
    }
    return true;
}

static bool reject_embedded_nul(const unsigned char *data, size_t size,
                                HTHLevelError *error)
{
    size_t column = 1U;
    size_t index = 0U;
    size_t line = 1U;

    while (index < size) {
        if (data[index] == (unsigned char)'\0') {
            return fail_at(error, line, column,
                           "embedded NUL byte is not allowed");
        }
        if (data[index] == (unsigned char)'\r') {
            index++;
            if (index < size && data[index] == (unsigned char)'\n') {
                index++;
            }
            line++;
            column = 1U;
        } else if (data[index] == (unsigned char)'\n') {
            index++;
            line++;
            column = 1U;
        } else {
            index++;
            column++;
        }
    }
    return true;
}

static bool decimal_float_syntax_is_valid(HTHLevelToken token)
{
    size_t cursor = 0U;
    size_t digit_count = 0U;

    if (cursor < token.length &&
        (token.data[cursor] == (unsigned char)'+' ||
         token.data[cursor] == (unsigned char)'-')) {
        cursor++;
    }
    while (cursor < token.length && token.data[cursor] >= (unsigned char)'0' &&
           token.data[cursor] <= (unsigned char)'9') {
        cursor++;
        digit_count++;
    }
    if (cursor < token.length && token.data[cursor] == (unsigned char)'.') {
        cursor++;
        while (cursor < token.length &&
               token.data[cursor] >= (unsigned char)'0' &&
               token.data[cursor] <= (unsigned char)'9') {
            cursor++;
            digit_count++;
        }
    }
    if (digit_count == 0U) {
        return false;
    }
    if (cursor < token.length &&
        (token.data[cursor] == (unsigned char)'e' ||
         token.data[cursor] == (unsigned char)'E')) {
        size_t exponent_digits = 0U;

        cursor++;
        if (cursor < token.length &&
            (token.data[cursor] == (unsigned char)'+' ||
             token.data[cursor] == (unsigned char)'-')) {
            cursor++;
        }
        while (cursor < token.length &&
               token.data[cursor] >= (unsigned char)'0' &&
               token.data[cursor] <= (unsigned char)'9') {
            cursor++;
            exponent_digits++;
        }
        if (exponent_digits == 0U) {
            return false;
        }
    }
    return cursor == token.length;
}

static bool numeric_locale_create(HTHNumericLocale *out_locale)
{
#if defined(_WIN32)
    *out_locale = _create_locale(LC_NUMERIC, "C");
#else
    *out_locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
#endif
    return *out_locale != (HTHNumericLocale)0;
}

static void numeric_locale_destroy(HTHNumericLocale locale)
{
#if defined(_WIN32)
    _free_locale(locale);
#else
    freelocale(locale);
#endif
}

static bool parse_float_token(HTHLevelToken token, HTHNumericLocale locale,
                              float *out_value, HTHLevelError *error)
{
    char *end;
    char *text;
    float value;

    if (!decimal_float_syntax_is_valid(token) ||
        token.length == SIZE_MAX) {
        return fail_at(error, token.line, token.column,
                       "invalid finite decimal float");
    }
    text = malloc(token.length + 1U);
    if (text == NULL) {
        return fail_at(error, token.line, token.column,
                       "float token allocation failed");
    }
    memcpy(text, token.data, token.length);
    text[token.length] = '\0';
    errno = 0;
#if defined(_WIN32)
    value = _strtof_l(text, &end, locale);
#else
    value = strtof_l(text, &end, locale);
#endif
    if (errno == ERANGE || end != text + token.length || !isfinite(value)) {
        free(text);
        return fail_at(error, token.line, token.column,
                       "float is outside the finite float range");
    }
    free(text);
    *out_value = value;
    return true;
}

static bool read_float(HTHLevelScanner *scanner, HTHNumericLocale locale,
                       float *out_value, HTHLevelError *error)
{
    HTHLevelToken token;

    if (!scanner_read_token(scanner, &token)) {
        return fail_at(error, scanner->line, scanner->column,
                       "expected finite decimal float");
    }
    return parse_float_token(token, locale, out_value, error);
}

static bool parse_visual(HTHLevelScanner *scanner,
                         HTHWorldVisualClass *out_visual,
                         HTHLevelError *error)
{
    HTHLevelToken token;

    if (!scanner_read_token(scanner, &token)) {
        return fail_at(error, scanner->line, scanner->column,
                       "expected visual class");
    }
    if (token_equals(token, "none")) {
        *out_visual = HTH_WORLD_VISUAL_NONE;
    } else if (token_equals(token, "floor")) {
        *out_visual = HTH_WORLD_VISUAL_FLOOR;
    } else if (token_equals(token, "wall")) {
        *out_visual = HTH_WORLD_VISUAL_WALL;
    } else if (token_equals(token, "box")) {
        *out_visual = HTH_WORLD_VISUAL_BOX;
    } else if (token_equals(token, "low_step")) {
        *out_visual = HTH_WORLD_VISUAL_LOW_STEP;
    } else if (token_equals(token, "limit_step")) {
        *out_visual = HTH_WORLD_VISUAL_LIMIT_STEP;
    } else if (token_equals(token, "high_ledge")) {
        *out_visual = HTH_WORLD_VISUAL_HIGH_LEDGE;
    } else if (token_equals(token, "platform")) {
        *out_visual = HTH_WORLD_VISUAL_PLATFORM;
    } else if (token_equals(token, "corner")) {
        *out_visual = HTH_WORLD_VISUAL_CORNER;
    } else if (token_equals(token, "corridor_corner")) {
        *out_visual = HTH_WORLD_VISUAL_CORRIDOR_CORNER;
    } else {
        return fail_at(error, token.line, token.column,
                       "unknown visual class");
    }
    return true;
}

static bool parse_collision_shape(HTHLevelScanner *scanner,
                                  HTHWorldCollisionShape *out_shape,
                                  HTHLevelError *error)
{
    HTHLevelToken token;

    if (!scanner_read_token(scanner, &token)) {
        return fail_at(error, scanner->line, scanner->column,
                       "expected collision shape");
    }
    if (token_equals(token, "none")) {
        *out_shape = HTH_WORLD_COLLISION_NONE;
    } else if (token_equals(token, "aabb")) {
        *out_shape = HTH_WORLD_COLLISION_AABB;
    } else {
        return fail_at(error, token.line, token.column,
                       "unknown collision shape");
    }
    return true;
}

static bool parse_render_shape(HTHLevelScanner *scanner,
                               HTHWorldRenderShape *out_shape,
                               HTHLevelError *error)
{
    HTHLevelToken token;

    if (!scanner_read_token(scanner, &token)) {
        return fail_at(error, scanner->line, scanner->column,
                       "expected render shape");
    }
    if (token_equals(token, "none")) {
        *out_shape = HTH_WORLD_RENDER_NONE;
    } else if (token_equals(token, "box")) {
        *out_shape = HTH_WORLD_RENDER_BOX;
    } else if (token_equals(token, "wedge")) {
        *out_shape = HTH_WORLD_RENDER_WEDGE;
    } else {
        return fail_at(error, token.line, token.column,
                       "unknown render shape");
    }
    return true;
}

static bool shape_flags_are_consistent(
    const HTHWorldStaticObject *object)
{
    bool collidable =
        (object->flags & HTH_WORLD_OBJECT_COLLIDABLE) != 0U;
    bool visible = (object->flags & HTH_WORLD_OBJECT_VISIBLE) != 0U;

    return (collidable
                ? object->collision_shape == HTH_WORLD_COLLISION_AABB
                : object->collision_shape == HTH_WORLD_COLLISION_NONE) &&
           (visible
                ? object->render_shape != HTH_WORLD_RENDER_NONE
                : object->render_shape == HTH_WORLD_RENDER_NONE);
}

static bool token_is_flag(HTHLevelToken token)
{
    return token_equals(token, "none") || token_equals(token, "collidable") ||
           token_equals(token, "visible");
}

static bool parse_flags_and_visual(HTHLevelScanner *scanner,
                                   uint32_t *out_flags,
                                   HTHWorldVisualClass *out_visual,
                                   HTHLevelError *error)
{
    HTHLevelToken first;
    HTHLevelToken next;
    uint32_t flags;

    if (!scanner_read_token(scanner, &first)) {
        return fail_at(error, scanner->line, scanner->column,
                       "expected object flags");
    }
    if (token_equals(first, "none")) {
        flags = 0U;
    } else if (token_equals(first, "collidable")) {
        flags = HTH_WORLD_OBJECT_COLLIDABLE;
    } else if (token_equals(first, "visible")) {
        flags = HTH_WORLD_OBJECT_VISIBLE;
    } else {
        return fail_at(error, first.line, first.column, "unknown object flag");
    }

    if (!scanner_read_token(scanner, &next)) {
        return fail_at(error, scanner->line, scanner->column,
                       "expected 'visual' after object flags");
    }
    if (token_equals(first, "collidable") && token_equals(next, "visible")) {
        flags |= HTH_WORLD_OBJECT_VISIBLE;
        if (!scanner_read_token(scanner, &next)) {
            return fail_at(error, scanner->line, scanner->column,
                           "expected 'visual' after object flags");
        }
    }
    if (!token_equals(next, "visual")) {
        if (token_is_flag(next)) {
            return fail_at(error, next.line, next.column,
                           "duplicate, noncanonical, or combined object flag");
        }
        return fail_at(error, next.line, next.column, "unknown object flag");
    }
    *out_flags = flags;
    return parse_visual(scanner, out_visual, error);
}

static bool append_object(HTHLevelDescription *description,
                          HTHLevelStaticObjectDescription object,
                          HTHLevelError *error)
{
    if (description->object_count == description->object_capacity) {
        size_t new_capacity = description->object_capacity == 0U
            ? 8U : description->object_capacity * 2U;
        HTHLevelStaticObjectDescription *new_objects;

        if (new_capacity < description->object_capacity ||
            new_capacity > SIZE_MAX / sizeof(*new_objects)) {
            return fail_at(error, object.source_line, object.source_column,
                           "level object count exceeds addressable storage");
        }
        new_objects = realloc(description->objects,
                              new_capacity * sizeof(*new_objects));
        if (new_objects == NULL) {
            return fail_at(error, object.source_line, object.source_column,
                           "level object allocation failed");
        }
        description->objects = new_objects;
        description->object_capacity = new_capacity;
    }
    description->objects[description->object_count++] = object;
    return true;
}

static bool parse_static_object(HTHLevelScanner *scanner,
                                HTHNumericLocale locale,
                                HTHLevelToken object_token,
                                HTHLevelDescription *description,
                                HTHLevelError *error)
{
    HTHLevelStaticObjectDescription object = {0};

    object.source_line = object_token.line;
    object.source_column = object_token.column;
    if (!expect_token(scanner, "bounds", "expected 'bounds'", error) ||
        !read_float(scanner, locale, &object.object.bounds.min.x, error) ||
        !read_float(scanner, locale, &object.object.bounds.min.y, error) ||
        !read_float(scanner, locale, &object.object.bounds.min.z, error) ||
        !read_float(scanner, locale, &object.object.bounds.max.x, error) ||
        !read_float(scanner, locale, &object.object.bounds.max.y, error) ||
        !read_float(scanner, locale, &object.object.bounds.max.z, error) ||
        !expect_token(scanner, "collision", "expected 'collision'", error) ||
        !parse_collision_shape(scanner, &object.object.collision_shape,
                               error) ||
        !expect_token(scanner, "render", "expected 'render'", error) ||
        !parse_render_shape(scanner, &object.object.render_shape, error) ||
        !expect_token(scanner, "flags", "expected 'flags'", error) ||
        !parse_flags_and_visual(scanner, &object.object.flags,
                                &object.object.visual_class, error) ||
        !expect_token(scanner, "end", "expected 'end'", error)) {
        return false;
    }
    if (!shape_flags_are_consistent(&object.object)) {
        return fail_at(error, object.source_line, object.source_column,
                       "object shapes are inconsistent with flags");
    }
    return append_object(description, object, error);
}

bool hth_level_parse(const unsigned char *data, size_t size,
                     HTHLevelDescription *out_description,
                     HTHLevelError *out_error)
{
    HTHLevelDescription parsed = {0};
    HTHLevelScanner scanner = {data, size, 0U, 1U, 1U};
    HTHNumericLocale locale = (HTHNumericLocale)0;
    HTHLevelToken token;
    bool success = false;

    clear_error(out_error);
    if (out_error == NULL || !description_is_empty(out_description) ||
        (data == NULL && size != 0U)) {
        return fail_at(out_error, 1U, 1U, "invalid level parser arguments");
    }
    if (!reject_embedded_nul(data, size, out_error) ||
        !numeric_locale_create(&locale)) {
        if (locale == (HTHNumericLocale)0 && out_error->message[0] == '\0') {
            (void)fail_at(out_error, 1U, 1U,
                          "C numeric locale initialization failed");
        }
        goto cleanup;
    }
    if (!expect_token(&scanner, "hthlevel", "expected 'hthlevel' header",
                      out_error) ||
        !scanner_read_token(&scanner, &token)) {
        if (out_error->message[0] == '\0') {
            (void)fail_at(out_error, scanner.line, scanner.column,
                          "expected level format version");
        }
        goto cleanup;
    }
    if (!token_equals(token, "2")) {
        (void)fail_at(out_error, token.line, token.column,
                      "unsupported level format version");
        goto cleanup;
    }
    parsed.format_version = HTH_LEVEL_FORMAT_VERSION;
    if (!expect_token(&scanner, "spawn", "expected exactly one 'spawn'",
                      out_error)) {
        goto cleanup;
    }
    parsed.spawn_line = scanner.line;
    parsed.spawn_column = scanner.column;
    if (!read_float(&scanner, locale, &parsed.default_spawn.position.x,
                    out_error) ||
        !read_float(&scanner, locale, &parsed.default_spawn.position.y,
                    out_error) ||
        !read_float(&scanner, locale, &parsed.default_spawn.position.z,
                    out_error) ||
        !read_float(&scanner, locale, &parsed.default_spawn.yaw_radians,
                    out_error)) {
        goto cleanup;
    }
    parsed.has_default_spawn = true;

    while (scanner_read_token(&scanner, &token)) {
        if (!token_equals(token, "static_object")) {
            (void)fail_at(out_error, token.line, token.column,
                          "expected 'static_object' or end of file");
            goto cleanup;
        }
        if (!parse_static_object(&scanner, locale, token, &parsed,
                                 out_error)) {
            goto cleanup;
        }
    }
    *out_description = parsed;
    memset(&parsed, 0, sizeof(parsed));
    success = true;

cleanup:
    if (locale != (HTHNumericLocale)0) {
        numeric_locale_destroy(locale);
    }
    hth_level_description_destroy(&parsed);
    return success;
}

static bool world_output_is_empty(const HTHWorld *world)
{
    return world != NULL && world->objects == NULL &&
           world->object_count == 0U && world->object_capacity == 0U &&
           !world->has_bounds && !world->has_default_spawn &&
           !world->finalized;
}

bool hth_level_build_world(const HTHLevelDescription *description,
                           HTHWorld *out_world, HTHLevelError *out_error)
{
    HTHWorld world = {0};
    size_t index;

    clear_error(out_error);
    if (out_error == NULL || description == NULL ||
        description->format_version != HTH_LEVEL_FORMAT_VERSION ||
        !description->has_default_spawn ||
        description->object_count > description->object_capacity ||
        (description->object_count > 0U && description->objects == NULL) ||
        !world_output_is_empty(out_world)) {
        return fail_at(out_error, 1U, 1U,
                       "invalid level description or World output");
    }
    if (!hth_world_init(&world) ||
        !hth_world_set_default_spawn(&world, description->default_spawn)) {
        hth_world_shutdown(&world);
        return fail_at(out_error, description->spawn_line,
                       description->spawn_column,
                       "World rejected the default spawn");
    }
    for (index = 0U; index < description->object_count; ++index) {
        const HTHLevelStaticObjectDescription *object =
            &description->objects[index];

        if (!hth_world_add_static_object(
                &world, object->object.bounds,
                object->object.collision_shape,
                object->object.render_shape, object->object.flags,
                object->object.visual_class)) {
            hth_world_shutdown(&world);
            return fail_at(out_error, object->source_line,
                           object->source_column,
                           "World rejected the static object");
        }
    }
    if (!hth_world_finalize(&world)) {
        hth_world_shutdown(&world);
        return fail_at(out_error, 1U, 1U, "World finalization failed");
    }
    *out_world = world;
    return true;
}
