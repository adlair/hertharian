#include "image.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

static HTHImageData decode_success(const unsigned char *data, size_t size)
{
    HTHImageData image = {0};
    HTHImageError error = {0};

    assert(hth_image_decode_ppm_p3(data, size, &image, &error));
    assert(error.message[0] == '\0');
    return image;
}

static void assert_decode_failure(const unsigned char *data, size_t size)
{
    HTHImageData image = {0};
    HTHImageError error = {0};

    assert(!hth_image_decode_ppm_p3(data, size, &image, &error));
    assert(image.pixels == NULL);
    assert(image.width == 0U);
    assert(image.height == 0U);
    assert(image.format == 0);
    assert(error.line > 0U);
    assert(error.column > 0U);
    assert(error.message[0] != '\0');
    hth_image_data_release(&image);
}

static void test_exact_pixels_and_row_order(void)
{
    static const unsigned char ppm[] =
        "P3\n2 1\n255\n255 0 0  0 255 0";
    static const unsigned char expected[] = {
        255U, 0U, 0U, 0U, 255U, 0U
    };
    HTHImageData image = decode_success(ppm, sizeof(ppm) - 1U);

    assert(image.width == 2U);
    assert(image.height == 1U);
    assert(image.format == HTH_IMAGE_FORMAT_RGB8);
    assert(memcmp(image.pixels, expected, sizeof(expected)) == 0);
    hth_image_data_release(&image);
    assert(image.pixels == NULL);
    assert(image.width == 0U);
    assert(image.height == 0U);
    assert(image.format == 0);
    hth_image_data_release(&image);
}

static void test_valid_layouts(void)
{
    static const unsigned char one_by_one[] =
        "# before\r\nP3\r\n1 1\r\n255\r\n0 128 255";
    static const unsigned char two_by_two[] =
        "P3 2 2 255 # pixels\n"
        "0 0 0  255 255 255\n"
        "255 0 0  0 0 255\n# trailing comment";
    static const unsigned char width_three[] =
        "P3\r3 2\r255\r"
        "0 0 0  1 2 3  4 5 6\r"
        "7 8 9  10 11 12  13 14 15\r";
    HTHImageData first =
        decode_success(one_by_one, sizeof(one_by_one) - 1U);
    HTHImageData second =
        decode_success(two_by_two, sizeof(two_by_two) - 1U);
    HTHImageData third =
        decode_success(width_three, sizeof(width_three) - 1U);

    assert(first.width == 1U && first.height == 1U);
    assert(second.width == 2U && second.height == 2U);
    assert(third.width == 3U && third.height == 2U);
    assert(third.pixels[17] == 15U);
    hth_image_data_release(&first);
    hth_image_data_release(&second);
    hth_image_data_release(&third);
}

static void test_invalid_images(void)
{
    static const char *const invalid[] = {
        "",
        "P6 1 1 255 0 0 0",
        "p3 1 1 255 0 0 0",
        "P3",
        "P3 0 1 255",
        "P3 1 0 255",
        "P3 -1 1 255 0 0 0",
        "P3 4294967296 1 255 0 0 0",
        "P3 4294967295 4294967295 255 0 0 0",
        "P3 1 1",
        "P3 1 1 254 0 0 0",
        "P3 1 1 256 0 0 0",
        "P3 1 1 255 -1 0 0",
        "P3 1 1 255 256 0 0",
        "P3 1 1 255 nope 0 0",
        "P3 1 1 255 0 0",
        "P3 1 1 255 0 0 0 1",
        "P3 1 1 255 0 0 0 garbage",
        "P3 1 1 255 0 0 2x"
    };
    static const unsigned char embedded_nul[] =
        "P3 1 1 255 0\0 0 0";
    static const unsigned char bom[] = {
        0xEFU, 0xBBU, 0xBFU, 'P', '3', ' ', '1', ' ', '1', ' ', '2',
        '5', '5', ' ', '0', ' ', '0', ' ', '0'
    };
    size_t index;

    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        assert_decode_failure((const unsigned char *)invalid[index],
                              strlen(invalid[index]));
    }
    assert_decode_failure(embedded_nul, sizeof(embedded_nul) - 1U);
    assert_decode_failure(bom, sizeof(bom));
}

static void test_arguments(void)
{
    static const unsigned char valid[] = "P3 1 1 255 0 0 0";
    HTHImageData image = {0};
    HTHImageData occupied = {0};
    HTHImageError error = {0};

    assert(!hth_image_decode_ppm_p3(NULL, 0U, &image, &error));
    assert(error.message[0] != '\0');
    assert(!hth_image_decode_ppm_p3(NULL, 1U, &image, &error));
    assert(!hth_image_decode_ppm_p3(valid, sizeof(valid) - 1U,
                                    NULL, &error));
    assert(!hth_image_decode_ppm_p3(valid, sizeof(valid) - 1U,
                                    &image, NULL));
    occupied.width = 1U;
    assert(!hth_image_decode_ppm_p3(valid, sizeof(valid) - 1U,
                                    &occupied, &error));
    assert(occupied.width == 1U);
    hth_image_data_release(NULL);
}

int main(void)
{
    test_exact_pixels_and_row_order();
    test_valid_layouts();
    test_invalid_images();
    test_arguments();
    return 0;
}
