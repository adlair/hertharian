#include "image.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const unsigned char *data;
    size_t size;
    size_t cursor;
    size_t line;
    size_t column;
} HTHImageScanner;

typedef struct {
    const unsigned char *data;
    size_t length;
    size_t line;
    size_t column;
} HTHImageToken;

static void clear_error(HTHImageError *error)
{
    if (error != NULL) {
        memset(error, 0, sizeof(*error));
    }
}

static bool fail_at(HTHImageError *error, size_t line, size_t column,
                    const char *message)
{
    if (error != NULL) {
        error->line = line;
        error->column = column;
        (void)snprintf(error->message, sizeof(error->message), "%s", message);
    }
    return false;
}

static bool image_is_empty(const HTHImageData *image)
{
    return image != NULL && image->pixels == NULL && image->width == 0U &&
           image->height == 0U && image->format == 0;
}

void hth_image_data_release(HTHImageData *image)
{
    if (image == NULL) {
        return;
    }
    free(image->pixels);
    memset(image, 0, sizeof(*image));
}

static bool token_equals(HTHImageToken token, const char *text)
{
    size_t length = strlen(text);

    return token.length == length && memcmp(token.data, text, length) == 0;
}

static void scanner_advance_newline(HTHImageScanner *scanner)
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

static void scanner_skip_whitespace_and_comments(HTHImageScanner *scanner)
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

static bool scanner_read_token(HTHImageScanner *scanner,
                               HTHImageToken *out_token)
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

static bool reject_embedded_nul(const unsigned char *data, size_t size,
                                HTHImageError *error)
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

static bool parse_uint32_token(HTHImageToken token, uint32_t *out_value,
                               HTHImageError *error, const char *message)
{
    uint32_t value = 0U;
    size_t index;

    if (token.length == 0U) {
        return fail_at(error, token.line, token.column, message);
    }
    for (index = 0U; index < token.length; ++index) {
        uint32_t digit;

        if (token.data[index] < (unsigned char)'0' ||
            token.data[index] > (unsigned char)'9') {
            return fail_at(error, token.line, token.column, message);
        }
        digit = (uint32_t)(token.data[index] - (unsigned char)'0');
        if (value > (UINT32_MAX - digit) / 10U) {
            return fail_at(error, token.line, token.column, message);
        }
        value = value * 10U + digit;
    }
    *out_value = value;
    return true;
}

static bool read_uint32(HTHImageScanner *scanner, uint32_t *out_value,
                        HTHImageError *error, const char *message)
{
    HTHImageToken token;

    if (!scanner_read_token(scanner, &token)) {
        return fail_at(error, scanner->line, scanner->column, message);
    }
    return parse_uint32_token(token, out_value, error, message);
}

bool hth_image_decode_ppm_p3(const unsigned char *data, size_t size,
                             HTHImageData *out_image,
                             HTHImageError *out_error)
{
    HTHImageData decoded = {0};
    HTHImageScanner scanner = {data, size, 0U, 1U, 1U};
    HTHImageToken token;
    uint32_t channel;
    uint32_t max_value;
    size_t channel_count;
    size_t index;
    size_t pixel_count;
    bool success = false;

    clear_error(out_error);
    if (out_error == NULL || !image_is_empty(out_image) ||
        (data == NULL && size != 0U)) {
        return fail_at(out_error, 1U, 1U, "invalid image decoder arguments");
    }
    if (!reject_embedded_nul(data, size, out_error)) {
        return false;
    }
    if (!scanner_read_token(&scanner, &token)) {
        return fail_at(out_error, scanner.line, scanner.column,
                       "expected PPM P3 magic");
    }
    if (!token_equals(token, "P3")) {
        return fail_at(out_error, token.line, token.column,
                       "expected PPM P3 magic");
    }
    if (!read_uint32(&scanner, &decoded.width, out_error,
                     "expected positive PPM width")) {
        goto cleanup;
    }
    if (decoded.width == 0U) {
        (void)fail_at(out_error, scanner.line, scanner.column,
                      "PPM width must be positive");
        goto cleanup;
    }
    if (!read_uint32(&scanner, &decoded.height, out_error,
                     "expected positive PPM height")) {
        goto cleanup;
    }
    if (decoded.height == 0U) {
        (void)fail_at(out_error, scanner.line, scanner.column,
                      "PPM height must be positive");
        goto cleanup;
    }
    if (!read_uint32(&scanner, &max_value, out_error,
                     "expected PPM maxval 255")) {
        goto cleanup;
    }
    if (max_value != 255U) {
        (void)fail_at(out_error, scanner.line, scanner.column,
                      "PPM maxval must be exactly 255");
        goto cleanup;
    }
    if ((size_t)decoded.width > SIZE_MAX / (size_t)decoded.height) {
        (void)fail_at(out_error, scanner.line, scanner.column,
                      "PPM dimensions exceed addressable storage");
        goto cleanup;
    }
    pixel_count = (size_t)decoded.width * (size_t)decoded.height;
    if (pixel_count > SIZE_MAX / 3U) {
        (void)fail_at(out_error, scanner.line, scanner.column,
                      "PPM pixel data exceeds addressable storage");
        goto cleanup;
    }
    channel_count = pixel_count * 3U;
    decoded.pixels = malloc(channel_count);
    if (decoded.pixels == NULL) {
        (void)fail_at(out_error, scanner.line, scanner.column,
                      "PPM pixel allocation failed");
        goto cleanup;
    }
    decoded.format = HTH_IMAGE_FORMAT_RGB8;
    for (index = 0U; index < channel_count; ++index) {
        if (!read_uint32(&scanner, &channel, out_error,
                         "expected PPM RGB channel integer")) {
            goto cleanup;
        }
        if (channel > 255U) {
            (void)fail_at(out_error, scanner.line, scanner.column,
                          "PPM RGB channel must be between 0 and 255");
            goto cleanup;
        }
        decoded.pixels[index] = (unsigned char)channel;
    }
    if (scanner_read_token(&scanner, &token)) {
        (void)fail_at(out_error, token.line, token.column,
                      "unexpected token after PPM pixel data");
        goto cleanup;
    }
    *out_image = decoded;
    memset(&decoded, 0, sizeof(decoded));
    success = true;

cleanup:
    hth_image_data_release(&decoded);
    return success;
}
