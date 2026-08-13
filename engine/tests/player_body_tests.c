#include "player_body.h"
#include "hth_camera.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>

static bool close_enough(float left, float right)
{
    return fabsf(left - right) <= 1.0e-5F;
}

int main(void)
{
    HTHAABB bounds;
    HTHCamera camera;
    HTHPlayerBody body;
    HTHVec3 body_position;

    assert(hth_player_body_init(&body, hth_vec3(2.0F, 1.0F, -3.0F)));
    assert(hth_player_body_is_valid(&body));
    assert(close_enough(body.half_width, 0.30F));
    assert(close_enough(body.height, 1.80F));
    assert(close_enough(body.eye_height, 1.60F));
    assert(!body.grounded);
    assert(hth_player_body_bounds(&body, &bounds));
    assert(close_enough(bounds.min.x, 1.70F));
    assert(close_enough(bounds.min.y, 1.0F));
    assert(close_enough(bounds.min.z, -3.30F));
    assert(close_enough(bounds.max.x, 2.30F));
    assert(close_enough(bounds.max.y, 2.80F));
    assert(close_enough(bounds.max.z, -2.70F));

    hth_camera_init_default(&camera);
    assert(hth_player_body_eye_position(&body, &camera.position));
    assert(close_enough(camera.position.x, 2.0F));
    assert(close_enough(camera.position.y, 2.60F));
    assert(close_enough(camera.position.z, -3.0F));
    body_position = body.position;
    camera.forward = hth_vec3(1.0F, 0.0F, 0.0F);
    assert(close_enough(body.position.x, body_position.x));
    assert(close_enough(body.position.y, body_position.y));
    assert(close_enough(body.position.z, body_position.z));
    body.position.x += 1.0F;
    assert(hth_player_body_eye_position(&body, &camera.position));
    assert(close_enough(camera.position.x, 3.0F));

    body.eye_height = body.height;
    assert(!hth_player_body_is_valid(&body));
    body.eye_height = 1.60F;
    body.half_width = 0.0F;
    assert(!hth_player_body_is_valid(&body));
    return 0;
}
