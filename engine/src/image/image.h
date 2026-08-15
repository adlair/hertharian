#ifndef HTH_IMAGE_H
#define HTH_IMAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HTH_IMAGE_ERROR_MESSAGE_CAPACITY 160U

typedef enum {
    HTH_IMAGE_FORMAT_RGB8 = 1
} HTHImageFormat;

typedef struct {
    unsigned char *pixels;
    uint32_t width;
    uint32_t height;
    HTHImageFormat format;
} HTHImageData;

typedef struct {
    size_t line;
    size_t column;
    char message[HTH_IMAGE_ERROR_MESSAGE_CAPACITY];
} HTHImageError;

bool hth_image_decode_ppm_p3(const unsigned char *data, size_t size,
                             HTHImageData *out_image,
                             HTHImageError *out_error);
void hth_image_data_release(HTHImageData *image);

#endif
