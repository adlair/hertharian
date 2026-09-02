#include "renderer.h"
#include "renderer_opengl.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,       \
                    __LINE__, #condition);                                   \
            return false;                                                    \
        }                                                                    \
    } while (0)

struct HTHPlatform {
    bool valid;
};

struct HTHOpenGLBackend {
    bool valid;
};

static struct HTHOpenGLBackend fake_backend = {true};
static HTHRendererTransientDraw observed_draws[4];
static size_t observed_count;
static unsigned int frame_calls;
static bool frame_success = true;

bool hth_platform_framebuffer_size(HTHPlatform *platform,
                                   uint32_t *out_width,
                                   uint32_t *out_height)
{
    if (platform == NULL || !platform->valid || out_width == NULL ||
        out_height == NULL) {
        return false;
    }
    *out_width = 1280U;
    *out_height = 720U;
    return true;
}

HTHOpenGLBackend *hth_renderer_opengl_create(
    HTHPlatform *platform, const HTHRendererStaticDraw *draws,
    size_t draw_count)
{
    if (platform == NULL || (draw_count > 0U && draws == NULL)) {
        return NULL;
    }
    return &fake_backend;
}

void hth_renderer_opengl_destroy(HTHOpenGLBackend *backend)
{
    (void)backend;
}

bool hth_renderer_opengl_resize(HTHOpenGLBackend *backend,
                                uint32_t width, uint32_t height)
{
    return backend == &fake_backend && width == 1280U && height == 720U;
}

bool hth_renderer_opengl_set_camera_matrices(HTHOpenGLBackend *backend,
                                             const HTHMat4 *view,
                                             const HTHMat4 *projection)
{
    return backend == &fake_backend && view != NULL && projection != NULL;
}

bool hth_renderer_opengl_frame(
    HTHOpenGLBackend *backend,
    const HTHRendererTransientDraw *runtime_draws,
    size_t runtime_draw_count)
{
    size_t index;

    frame_calls++;
    observed_count = runtime_draw_count;
    if (backend != &fake_backend || runtime_draw_count > 4U ||
        (runtime_draw_count > 0U && runtime_draws == NULL)) {
        return false;
    }
    for (index = 0U; index < runtime_draw_count; ++index) {
        observed_draws[index] = runtime_draws[index];
    }
    return frame_success;
}

static HTHRendererTransientDraw make_draw(float x)
{
    HTHRendererTransientDraw draw = {
        HTH_GEOMETRY_PRIMITIVE_BOX,
        {{0.0F}},
        {1.0F, 1.0F, 1.0F, 1.0F}
    };

    draw.model = hth_mat4_translation(hth_vec3(x, 2.0F, 3.0F));
    return draw;
}

static bool test_transient_forwarding_and_failure(void)
{
    struct HTHPlatform platform = {true};
    HTHCamera camera;
    HTHRenderer *renderer;
    HTHRendererTransientDraw draws[2] = {make_draw(1.0F), make_draw(4.0F)};
    unsigned int calls_before;

    hth_camera_init_default(&camera);
    renderer = hth_renderer_create(&platform, &camera, NULL, 0U);
    CHECK(renderer != NULL);
    CHECK(hth_renderer_frame(renderer, NULL, 0U));
    CHECK(frame_calls == 1U && observed_count == 0U);
    calls_before = frame_calls;
    CHECK(!hth_renderer_frame(renderer, NULL, 1U));
    CHECK(frame_calls == calls_before);
    CHECK(hth_renderer_frame(renderer, draws, 1U));
    CHECK(observed_count == 1U &&
          observed_draws[0].model.elements[12] == 1.0F);
    draws[0].model.elements[12] = 99.0F;
    CHECK(observed_draws[0].model.elements[12] == 1.0F);
    CHECK(hth_renderer_frame(renderer, draws, 2U));
    CHECK(observed_count == 2U &&
          observed_draws[0].model.elements[12] == 99.0F &&
          observed_draws[1].model.elements[12] == 4.0F);
    frame_success = false;
    CHECK(!hth_renderer_frame(renderer, draws, 1U));
    frame_success = true;
    CHECK(!hth_renderer_frame(NULL, NULL, 0U));
    hth_renderer_destroy(renderer);
    return true;
}

int main(void)
{
    if (!test_transient_forwarding_and_failure()) {
        return EXIT_FAILURE;
    }
    puts("Renderer frontend transient draw tests passed.");
    return EXIT_SUCCESS;
}
