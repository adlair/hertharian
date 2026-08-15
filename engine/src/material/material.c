#if !defined(_WIN32)
#define _GNU_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include "material.h"
#include "resource.h"

#include <errno.h>
#include <locale.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
typedef _locale_t HTHMaterialNumericLocale;
#else
typedef locale_t HTHMaterialNumericLocale;
#endif

typedef struct {
    const unsigned char *data;
    size_t size;
    size_t cursor;
    size_t line;
    size_t column;
} HTHMaterialScanner;

typedef struct {
    const unsigned char *data;
    size_t length;
    size_t line;
    size_t column;
} HTHMaterialToken;

static void clear_error(HTHMaterialError *error)
{
    if (error != NULL) {
        memset(error, 0, sizeof(*error));
    }
}

static bool fail_at(HTHMaterialError *error, size_t line, size_t column,
                    const char *message)
{
    if (error != NULL) {
        error->line = line;
        error->column = column;
        (void)snprintf(error->message, sizeof(error->message), "%s", message);
    }
    return false;
}

static bool description_is_empty(const HTHMaterialDescription *description)
{
    return description != NULL && description->format_version == 0U &&
           description->texture_resource_id == NULL &&
           !description->has_texture;
}

void hth_material_description_destroy(HTHMaterialDescription *description)
{
    if (description == NULL) {
        return;
    }
    free(description->texture_resource_id);
    memset(description, 0, sizeof(*description));
}

static bool token_equals(HTHMaterialToken token, const char *text)
{
    size_t length = strlen(text);

    return token.length == length && memcmp(token.data, text, length) == 0;
}

static void scanner_advance_newline(HTHMaterialScanner *scanner)
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

static void scanner_skip_whitespace_and_comments(HTHMaterialScanner *scanner)
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

static bool scanner_read_token(HTHMaterialScanner *scanner,
                               HTHMaterialToken *out_token)
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

static bool expect_token(HTHMaterialScanner *scanner, const char *expected,
                         const char *message, HTHMaterialError *error)
{
    HTHMaterialToken token;

    if (!scanner_read_token(scanner, &token)) {
        return fail_at(error, scanner->line, scanner->column, message);
    }
    if (!token_equals(token, expected)) {
        return fail_at(error, token.line, token.column, message);
    }
    return true;
}

static bool reject_embedded_nul(const unsigned char *data, size_t size,
                                HTHMaterialError *error)
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

static bool decimal_float_syntax_is_valid(HTHMaterialToken token)
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

static bool numeric_locale_create(HTHMaterialNumericLocale *out_locale)
{
#if defined(_WIN32)
    *out_locale = _create_locale(LC_NUMERIC, "C");
#else
    *out_locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
#endif
    return *out_locale != (HTHMaterialNumericLocale)0;
}

static void numeric_locale_destroy(HTHMaterialNumericLocale locale)
{
#if defined(_WIN32)
    _free_locale(locale);
#else
    freelocale(locale);
#endif
}

static bool parse_float_token(HTHMaterialToken token,
                              HTHMaterialNumericLocale locale,
                              float *out_value, HTHMaterialError *error)
{
    char *end;
    char *text;
    float value;

    if (!decimal_float_syntax_is_valid(token) || token.length == SIZE_MAX) {
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

static bool read_color_component(HTHMaterialScanner *scanner,
                                 HTHMaterialNumericLocale locale,
                                 float *out_value,
                                 HTHMaterialError *error)
{
    HTHMaterialToken token;

    if (!scanner_read_token(scanner, &token)) {
        return fail_at(error, scanner->line, scanner->column,
                       "expected base_color component");
    }
    if (!parse_float_token(token, locale, out_value, error)) {
        return false;
    }
    if (*out_value < 0.0F || *out_value > 1.0F) {
        return fail_at(error, token.line, token.column,
                       "base_color component must be between 0 and 1");
    }
    return true;
}

static bool copy_texture_id(HTHMaterialToken token,
                            HTHMaterialDescription *description,
                            HTHMaterialError *error)
{
    char *resource_id;

    if (token.length == SIZE_MAX) {
        return fail_at(error, token.line, token.column,
                       "texture Resource ID is too long");
    }
    resource_id = malloc(token.length + 1U);
    if (resource_id == NULL) {
        return fail_at(error, token.line, token.column,
                       "texture Resource ID allocation failed");
    }
    memcpy(resource_id, token.data, token.length);
    resource_id[token.length] = '\0';
    if (!hth_resource_id_is_valid(resource_id)) {
        free(resource_id);
        return fail_at(error, token.line, token.column,
                       "invalid texture Resource ID");
    }
    description->texture_resource_id = resource_id;
    description->has_texture = true;
    return true;
}

bool hth_material_parse(const unsigned char *data, size_t size,
                        HTHMaterialDescription *out_description,
                        HTHMaterialError *out_error)
{
    HTHMaterialDescription parsed = {0};
    HTHMaterialScanner scanner = {data, size, 0U, 1U, 1U};
    HTHMaterialNumericLocale locale = (HTHMaterialNumericLocale)0;
    HTHMaterialToken token;
    bool success = false;
    size_t component;

    clear_error(out_error);
    if (out_error == NULL || !description_is_empty(out_description) ||
        (data == NULL && size != 0U)) {
        return fail_at(out_error, 1U, 1U, "invalid material parser arguments");
    }
    if (!reject_embedded_nul(data, size, out_error) ||
        !numeric_locale_create(&locale)) {
        if (locale == (HTHMaterialNumericLocale)0 &&
            out_error->message[0] == '\0') {
            (void)fail_at(out_error, 1U, 1U,
                          "C numeric locale initialization failed");
        }
        goto cleanup;
    }
    if (!expect_token(&scanner, "hthmaterial",
                      "expected 'hthmaterial' header", out_error) ||
        !scanner_read_token(&scanner, &token)) {
        if (out_error->message[0] == '\0') {
            (void)fail_at(out_error, scanner.line, scanner.column,
                          "expected material format version");
        }
        goto cleanup;
    }
    if (!token_equals(token, "1")) {
        (void)fail_at(out_error, token.line, token.column,
                      "unsupported material format version");
        goto cleanup;
    }
    parsed.format_version = HTH_MATERIAL_FORMAT_VERSION;
    if (!expect_token(&scanner, "base_color", "expected 'base_color'",
                      out_error)) {
        goto cleanup;
    }
    for (component = 0U; component < 4U; ++component) {
        if (!read_color_component(&scanner, locale,
                                  &parsed.base_color[component], out_error)) {
            goto cleanup;
        }
    }
    if (!expect_token(&scanner, "texture", "expected 'texture'", out_error) ||
        !scanner_read_token(&scanner, &token)) {
        if (out_error->message[0] == '\0') {
            (void)fail_at(out_error, scanner.line, scanner.column,
                          "expected texture Resource ID or 'none'");
        }
        goto cleanup;
    }
    if (!token_equals(token, "none") &&
        !copy_texture_id(token, &parsed, out_error)) {
        goto cleanup;
    }
    if (scanner_read_token(&scanner, &token)) {
        (void)fail_at(out_error, token.line, token.column,
                      "unexpected token after material definition");
        goto cleanup;
    }
    *out_description = parsed;
    memset(&parsed, 0, sizeof(parsed));
    success = true;

cleanup:
    if (locale != (HTHMaterialNumericLocale)0) {
        numeric_locale_destroy(locale);
    }
    hth_material_description_destroy(&parsed);
    return success;
}
