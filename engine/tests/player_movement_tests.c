#include "event.h"
#include "input_internal.h"
#include "keyboard_reconciliation.h"
#include "player_movement.h"
#include "collision_trace.h"

#include <assert.h>
#include <math.h>

static bool close_enough(float left, float right)
{
    return fabsf(left - right) <= 1.0e-4F;
}

static HTHCollisionWorld floor_collision_world(void)
{
    HTHCollisionWorld collision = {0};

    collision.obstacles[0] = (HTHAABB){
        {-20.0F, -1.0F, -20.0F}, {20.0F, 0.0F, 20.0F}
    };
    collision.obstacle_count = 1U;
    assert(hth_collision_world_is_valid(&collision));
    return collision;
}

static bool body_penetrates(const HTHCollisionWorld *world,
                            const HTHPlayerBody *body)
{
    HTHTrace trace;
    HTHVec3 mins = {-body->half_width, 0.0F, -body->half_width};
    HTHVec3 maxs = {body->half_width, body->height, body->half_width};

    assert(hth_collision_world_trace_aabb(
        world, body->position, body->position, mins, maxs, &trace));
    return trace.start_solid;
}

static HTHCollisionWorld floor_and_box_world(void)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] = (HTHAABB){
        {-10.0F, -1.0F, -10.0F}, {10.0F, 0.0F, 10.0F}
    };
    world.obstacles[1] = (HTHAABB){
        {-1.0F, 0.0F, -2.0F}, {1.0F, 1.0F, -1.0F}
    };
    world.obstacle_count = 2;
    assert(hth_collision_world_is_valid(&world));
    return world;
}

static HTHCollisionWorld floor_and_step_world(float height)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] = (HTHAABB){
        {-10.0F, -1.0F, -10.0F}, {10.0F, 0.0F, 10.0F}
    };
    world.obstacles[1] = (HTHAABB){
        {-1.0F, 0.0F, -1.0F}, {1.0F, height, -0.4F}
    };
    world.obstacle_count = 2;
    assert(hth_collision_world_is_valid(&world));
    return world;
}

static void inject_key(HTHInput *input, HTHKey key)
{
    HTHPlatformEvent event = {0};
    event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
    event.data.keyboard.key = key;
    hth_input_handle_event(input, &event);
}

static void inject_repeat_key(HTHInput *input, HTHKey key)
{
    HTHPlatformEvent event = {0};
    event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
    event.data.keyboard.key = key;
    event.data.keyboard.repeat = true;
    hth_input_handle_event(input, &event);
}

static void release_key(HTHInput *input, HTHKey key)
{
    HTHPlatformEvent event = {0};
    event.type = HTH_PLATFORM_EVENT_KEY_UP;
    event.data.keyboard.key = key;
    hth_input_handle_event(input, &event);
}

static HTHPlayerMovementIntent intent_for(HTHKey first, HTHKey second,
                                          HTHVec3 forward, bool enabled)
{
    HTHInput *input = hth_input_create();
    HTHPlayerMovementIntent intent;

    assert(input != NULL);
    hth_input_begin_frame(input);
    if (first != HTH_KEY_UNKNOWN) {
        inject_key(input, first);
    }
    if (second != HTH_KEY_UNKNOWN) {
        inject_key(input, second);
    }
    assert(hth_player_movement_build_intent(
        input, forward, hth_vec3(0.0F, 1.0F, 0.0F), enabled, &intent));
    hth_input_destroy(input);
    return intent;
}

static void test_intent(void)
{
    HTHVec3 forward = hth_vec3(0.0F, 0.0F, -1.0F);
    HTHPlayerMovementIntent w = intent_for(HTH_KEY_W, HTH_KEY_UNKNOWN,
                                           forward, true);
    HTHPlayerMovementIntent s = intent_for(HTH_KEY_S, HTH_KEY_UNKNOWN,
                                           forward, true);
    HTHPlayerMovementIntent a = intent_for(HTH_KEY_A, HTH_KEY_UNKNOWN,
                                           forward, true);
    HTHPlayerMovementIntent d = intent_for(HTH_KEY_D, HTH_KEY_UNKNOWN,
                                           forward, true);
    HTHPlayerMovementIntent diagonal = intent_for(HTH_KEY_W, HTH_KEY_D,
                                                  forward, true);
    HTHPlayerMovementIntent pitched = intent_for(
        HTH_KEY_W, HTH_KEY_UNKNOWN, hth_vec3(0.0F, 0.8F, -0.6F), true);
    HTHPlayerMovementIntent disabled = intent_for(
        HTH_KEY_W, HTH_KEY_UNKNOWN, forward, false);

    assert(close_enough(w.direction.z, -1.0F));
    assert(close_enough(s.direction.z, 1.0F));
    assert(close_enough(a.direction.x, -1.0F));
    assert(close_enough(d.direction.x, 1.0F));
    assert(close_enough(hth_vec3_length(diagonal.direction), 1.0F));
    assert(close_enough(diagonal.magnitude, 1.0F));
    assert(close_enough(w.magnitude, diagonal.magnitude));
    assert(diagonal.direction.x > 0.0F && diagonal.direction.z < 0.0F);
    assert(close_enough(pitched.direction.y, 0.0F));
    assert(close_enough(pitched.direction.z, -1.0F));
    assert(close_enough(disabled.direction.x, 0.0F));
    assert(close_enough(disabled.direction.z, 0.0F));
    assert(close_enough(disabled.magnitude, 0.0F));
}

static void test_normalized_release_clears_directional_intent(void)
{
    const HTHKey movement_keys[] = {
        HTH_KEY_W, HTH_KEY_A, HTH_KEY_S, HTH_KEY_D
    };
    size_t index;

    for (index = 0;
         index < sizeof(movement_keys) / sizeof(movement_keys[0]); ++index) {
        HTHKeyboardReconciliation reconciliation = {0};
        bool physical_down[HTH_KEY_COUNT] = {false};
        HTHPlayerMovementIntent intent;
        HTHPlatformEvent release = {0};
        HTHInput *input = hth_input_create();
        HTHKey released;

        assert(input != NULL);
        hth_input_begin_frame(input);
        inject_key(input, movement_keys[index]);
        hth_keyboard_reconciliation_report_down(
            &reconciliation, movement_keys[index]);
        assert(hth_player_movement_build_intent(
            input, hth_vec3(0.0F, 0.0F, -1.0F),
            hth_vec3(0.0F, 1.0F, 0.0F), true, &intent));
        assert(intent.magnitude > 0.0F);

        assert(!hth_keyboard_reconciliation_next_release(
            &reconciliation, physical_down, &released));
        assert(hth_player_movement_build_intent(
            input, hth_vec3(0.0F, 0.0F, -1.0F),
            hth_vec3(0.0F, 1.0F, 0.0F), true, &intent));
        assert(intent.magnitude > 0.0F);
        assert(hth_keyboard_reconciliation_next_release(
            &reconciliation, physical_down, &released));
        release.type = HTH_PLATFORM_EVENT_KEY_UP;
        release.data.keyboard.key = released;
        hth_input_handle_event(input, &release);
        assert(hth_player_movement_build_intent(
            input, hth_vec3(0.0F, 0.0F, -1.0F),
            hth_vec3(0.0F, 1.0F, 0.0F), true, &intent));
        assert(close_enough(intent.magnitude, 0.0F));
        assert(!hth_input_key_down(input, movement_keys[index]));
        hth_input_destroy(input);
    }
}

static void test_repeat_restores_directional_intent_after_reconciliation(void)
{
    static const HTHKey movement_keys[] = {
        HTH_KEY_W, HTH_KEY_A, HTH_KEY_S, HTH_KEY_D
    };
    size_t index;

    for (index = 0U; index < sizeof(movement_keys) /
                                sizeof(movement_keys[0]); ++index) {
        HTHKeyboardReconciliation reconciliation = {0};
        bool observed_down[HTH_KEY_COUNT] = {false};
        HTHPlayerMovementIntent intent;
        HTHInput *input = hth_input_create();
        HTHKey released = HTH_KEY_UNKNOWN;

        assert(input != NULL);
        hth_input_begin_frame(input);
        inject_key(input, movement_keys[index]);
        hth_keyboard_reconciliation_report_down(
            &reconciliation, movement_keys[index]);
        assert(!hth_keyboard_reconciliation_next_release(
            &reconciliation, observed_down, &released));
        assert(hth_keyboard_reconciliation_next_release(
            &reconciliation, observed_down, &released));
        assert(released == movement_keys[index]);
        release_key(input, released);
        assert(!hth_input_key_down(input, movement_keys[index]));

        hth_input_begin_frame(input);
        hth_keyboard_reconciliation_report_down(
            &reconciliation, movement_keys[index]);
        inject_repeat_key(input, movement_keys[index]);
        assert(hth_input_key_down(input, movement_keys[index]));
        assert(!hth_input_key_pressed(input, movement_keys[index]));
        assert(hth_player_movement_build_intent(
            input, hth_vec3(0.0F, 0.0F, -1.0F),
            hth_vec3(0.0F, 1.0F, 0.0F), true, &intent));
        assert(intent.magnitude > 0.0F);
        hth_input_destroy(input);
    }
}

static void test_repeat_restores_diagonal_intent_after_reconciliation(void)
{
    HTHKeyboardReconciliation reconciliation = {0};
    bool observed_down[HTH_KEY_COUNT] = {false};
    HTHPlayerMovementIntent intent;
    HTHInput *input = hth_input_create();
    HTHKey released = HTH_KEY_UNKNOWN;
    unsigned int release_count = 0U;

    assert(input != NULL);
    hth_input_begin_frame(input);
    inject_key(input, HTH_KEY_W);
    inject_key(input, HTH_KEY_D);
    hth_keyboard_reconciliation_report_down(&reconciliation, HTH_KEY_W);
    hth_keyboard_reconciliation_report_down(&reconciliation, HTH_KEY_D);
    assert(!hth_keyboard_reconciliation_next_release(
        &reconciliation, observed_down, &released));
    while (hth_keyboard_reconciliation_next_release(
               &reconciliation, observed_down, &released)) {
        release_key(input, released);
        release_count++;
    }
    assert(release_count == 2U);
    assert(!hth_input_key_down(input, HTH_KEY_W));
    assert(!hth_input_key_down(input, HTH_KEY_D));

    hth_input_begin_frame(input);
    hth_keyboard_reconciliation_report_down(&reconciliation, HTH_KEY_W);
    inject_repeat_key(input, HTH_KEY_W);
    hth_keyboard_reconciliation_report_down(&reconciliation, HTH_KEY_D);
    inject_repeat_key(input, HTH_KEY_D);
    assert(hth_player_movement_build_intent(
        input, hth_vec3(0.0F, 0.0F, -1.0F),
        hth_vec3(0.0F, 1.0F, 0.0F), true, &intent));
    assert(intent.magnitude > 0.0F);
    assert(intent.direction.x > 0.0F && intent.direction.z < 0.0F);
    assert(!hth_input_key_pressed(input, HTH_KEY_W));
    assert(!hth_input_key_pressed(input, HTH_KEY_D));
    hth_input_destroy(input);
}

static void test_jump_intent_uses_pressed_transition(void)
{
    HTHInput *input = hth_input_create();
    HTHPlayerMovementIntent intent;
    HTHVec3 forward = hth_vec3(0.0F, 0.0F, -1.0F);

    assert(input != NULL);
    hth_input_begin_frame(input);
    inject_key(input, HTH_KEY_SPACE);
    assert(hth_player_movement_build_intent(
        input, forward, hth_vec3(0.0F, 1.0F, 0.0F), true, &intent));
    assert(intent.jump_pressed);

    hth_input_begin_frame(input);
    assert(hth_input_key_down(input, HTH_KEY_SPACE));
    assert(hth_player_movement_build_intent(
        input, forward, hth_vec3(0.0F, 1.0F, 0.0F), true, &intent));
    assert(!intent.jump_pressed);

    release_key(input, HTH_KEY_SPACE);
    hth_input_begin_frame(input);
    inject_key(input, HTH_KEY_SPACE);
    assert(hth_player_movement_build_intent(
        input, forward, hth_vec3(0.0F, 1.0F, 0.0F), true, &intent));
    assert(intent.jump_pressed);
    assert(hth_player_movement_build_intent(
        input, forward, hth_vec3(0.0F, 1.0F, 0.0F), false, &intent));
    assert(!intent.jump_pressed);
    hth_input_destroy(input);
}

static void test_integration(void)
{
    HTHCollisionWorld world;
    HTHPlayerBody body;
    HTHPlayerMovementIntent forward = {
        {0.0F, 0.0F, -1.0F}, 1.0F, false
    };
    HTHPlayerMovementIntent none = {
        {0.0F, 0.0F, 0.0F}, 0.0F, false
    };

    world = floor_collision_world();
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 5.0F, 3.0F)));
    assert(hth_player_movement_step(&body, &world, &forward, 0.05));
    assert(body.position.z < 3.0F);
    assert(body.velocity.z < 0.0F);
    assert(body.velocity.z > -3.0F);
    assert(close_enough(body.velocity.y, -0.4905F));
    assert(close_enough(body.position.y, 4.975475F));
    assert(!body.grounded);

    body.position = hth_vec3(0.0F, 5.0F, 3.0F);
    body.velocity = hth_vec3(0.0F, 0.0F, 0.0F);
    assert(hth_player_movement_step(&body, &world, &forward, 1.0));
    assert(body.position.z < 3.0F);
    assert(close_enough(body.velocity.y, -0.981F));
    assert(close_enough(body.position.y, 4.9019F));

    assert(hth_player_movement_step(&body, &world, &none, 0.0));
    assert(body.velocity.z < 0.0F);
}

static void test_gravity_lands_on_floor(void)
{
    HTHCollisionWorld world;
    HTHPlayerBody body;
    HTHPlayerMovementIntent none = {
        {0.0F, 0.0F, 0.0F}, 0.0F, false
    };
    unsigned int step;

    world = floor_collision_world();
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 1.0F, 3.0F)));
    for (step = 0; step < 120 && !body.grounded; ++step) {
        assert(hth_player_movement_step(&body, &world, &none, 1.0 / 60.0));
    }
    assert(body.grounded);
    assert(close_enough(body.position.y, 0.0F));
    assert(close_enough(body.velocity.y, 0.0F));
    assert(!body_penetrates(&world, &body));
    assert(hth_player_movement_step(&body, &world, &none, 1.0 / 60.0));
    assert(body.grounded);
    assert(close_enough(body.position.y, 0.0F));
}

static void test_repeated_side_push_does_not_step_up(void)
{
    HTHCollisionWorld world = floor_and_box_world();
    HTHPlayerBody body;
    HTHPlayerMovementIntent forward = {
        {0.0F, 0.0F, -1.0F}, 1.0F, false
    };
    unsigned int step;

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = true;
    for (step = 0; step < 240; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &forward, 1.0 / 60.0));
        assert(close_enough(body.position.y, 0.0F));
        assert(!body_penetrates(&world, &body));
    }
    assert(close_enough(body.position.z, -0.7F));
    assert(body.grounded);
}

static void test_diagonal_push_slides_without_step_up(void)
{
    HTHCollisionWorld world = floor_and_box_world();
    HTHPlayerBody body;
    HTHPlayerMovementIntent diagonal = {
        {0.70710677F, 0.0F, -0.70710677F}, 1.0F, false
    };
    unsigned int step;

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = true;
    for (step = 0; step < 120; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &diagonal, 1.0 / 60.0));
        assert(close_enough(body.position.y, 0.0F));
        assert(!body_penetrates(&world, &body));
    }
    assert(body.position.x > 1.3F);
    assert(body.position.z < -0.7F);
}

static void test_true_box_landing_from_above(void)
{
    HTHCollisionWorld world = floor_and_box_world();
    HTHPlayerBody body;
    HTHPlayerMovementIntent none = {
        {0.0F, 0.0F, 0.0F}, 0.0F, false
    };
    unsigned int step;

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 3.0F, -1.5F)));
    for (step = 0; step < 240 && !body.grounded; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &none, 1.0 / 60.0));
    }
    assert(body.grounded);
    assert(close_enough(body.position.y, 1.0F));
    assert(close_enough(body.velocity.y, 0.0F));
    assert(!body_penetrates(&world, &body));
}

static void test_floor_ground_stability(void)
{
    HTHCollisionWorld world = floor_and_box_world();
    HTHPlayerBody body;
    HTHPlayerMovementIntent none = {
        {0.0F, 0.0F, 0.0F}, 0.0F, false
    };
    unsigned int step;

    assert(hth_player_body_init(&body, hth_vec3(3.0F, 1.0F, 0.0F)));
    for (step = 0; step < 240 && !body.grounded; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &none, 1.0 / 60.0));
    }
    assert(body.grounded);
    for (step = 0; step < 300; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &none, 1.0 / 60.0));
        assert(close_enough(body.position.y, 0.0F));
        assert(close_enough(body.velocity.y, 0.0F));
        assert(body.grounded);
    }
}

static void test_wall_slide_and_repeated_push(void)
{
    HTHCollisionWorld world = {0};
    HTHPlayerBody body;
    HTHPlayerMovementIntent diagonal = {
        {0.70710677F, 0.0F, -0.70710677F}, 1.0F, false
    };
    HTHPlayerMovementIntent into_wall = {
        {1.0F, 0.0F, 0.0F}, 1.0F, false
    };
    unsigned int step;

    world.obstacles[0] = (HTHAABB){
        {-10.0F, -1.0F, -10.0F}, {10.0F, 0.0F, 10.0F}
    };
    world.obstacles[1] = (HTHAABB){
        {1.0F, 0.0F, -10.0F}, {2.0F, 3.0F, 10.0F}
    };
    world.obstacle_count = 2;
    assert(hth_player_body_init(&body, hth_vec3(0.6F, 0.0F, 0.0F)));
    body.grounded = true;
    for (step = 0; step < 30; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &diagonal, 1.0 / 60.0));
    }
    assert(close_enough(body.position.x, 0.7F));
    assert(body.position.z < -1.0F);
    assert(!body_penetrates(&world, &body));
    for (step = 0; step < 300; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &into_wall, 1.0 / 60.0));
        assert(close_enough(body.position.x, 0.7F));
        assert(close_enough(body.position.y, 0.0F));
        assert(!body_penetrates(&world, &body));
    }
}

static void test_inside_corner_stability(void)
{
    HTHCollisionWorld world = {0};
    HTHPlayerBody body;
    HTHPlayerMovementIntent diagonal = {
        {0.70710677F, 0.0F, -0.70710677F}, 1.0F, false
    };
    unsigned int step;

    world.obstacles[0] = (HTHAABB){
        {-10.0F, -1.0F, -10.0F}, {10.0F, 0.0F, 10.0F}
    };
    world.obstacles[1] = (HTHAABB){
        {1.0F, 0.0F, -10.0F}, {2.0F, 3.0F, 10.0F}
    };
    world.obstacles[2] = (HTHAABB){
        {-10.0F, 0.0F, -2.0F}, {2.0F, 3.0F, -1.0F}
    };
    world.obstacle_count = 3;
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = true;
    for (step = 0; step < 300; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &diagonal, 1.0 / 60.0));
        assert(!body_penetrates(&world, &body));
    }
    assert(close_enough(body.position.x, 0.7F));
    assert(close_enough(body.position.z, -0.7F));
    assert(close_enough(body.velocity.x, 0.0F));
    assert(close_enough(body.velocity.z, 0.0F));
}

static void assert_step_height_result(float obstacle_height,
                                      bool should_step)
{
    HTHCollisionWorld world = floor_and_step_world(obstacle_height);
    HTHPlayerBody body;
    HTHPlayerMovementIntent forward = {
        {0.0F, 0.0F, -1.0F}, 1.0F, false
    };

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = true;
    assert(hth_player_movement_step(&body, &world, &forward, 0.1));
    assert(!body_penetrates(&world, &body));
    if (should_step) {
        assert(close_enough(body.position.y, obstacle_height));
        assert(body.position.z < -0.1F);
        assert(body.grounded);
    } else {
        assert(close_enough(body.position.y, 0.0F));
        assert(close_enough(body.position.z, -0.1F));
    }
}

static void test_low_exact_and_high_steps(void)
{
    assert_step_height_result(0.20F, true);
    assert_step_height_result(0.30F, true);
    assert_step_height_result(0.60F, false);
}

static void test_step_requires_head_clearance(void)
{
    HTHCollisionWorld world = floor_and_step_world(0.20F);
    HTHPlayerBody body;
    HTHPlayerMovementIntent forward = {
        {0.0F, 0.0F, -1.0F}, 1.0F, false
    };

    world.obstacles[2] = (HTHAABB){
        {-2.0F, 1.90F, -2.0F}, {2.0F, 2.20F, 1.0F}
    };
    world.obstacle_count = 3;
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = true;
    assert(hth_player_movement_step(&body, &world, &forward, 0.1));
    assert(close_enough(body.position.y, 0.0F));
    assert(close_enough(body.position.z, -0.1F));
    assert(!body_penetrates(&world, &body));
}

static void test_step_requires_better_progress(void)
{
    HTHCollisionWorld world = floor_and_step_world(0.20F);
    HTHPlayerBody body;
    HTHPlayerMovementIntent forward = {
        {0.0F, 0.0F, -1.0F}, 1.0F, false
    };

    world.obstacles[2] = (HTHAABB){
        {-1.0F, 0.0F, -1.0F}, {1.0F, 3.0F, -0.4F}
    };
    world.obstacle_count = 3;
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = true;
    assert(hth_player_movement_step(&body, &world, &forward, 0.1));
    assert(close_enough(body.position.y, 0.0F));
    assert(close_enough(body.position.z, -0.1F));
    assert(!body_penetrates(&world, &body));
}

static void test_airborne_body_does_not_step(void)
{
    HTHCollisionWorld world = floor_and_step_world(0.20F);
    HTHPlayerBody body;
    HTHPlayerMovementIntent forward = {
        {0.0F, 0.0F, -1.0F}, 1.0F, false
    };

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.05F, 0.0F)));
    body.grounded = false;
    assert(hth_player_movement_step(&body, &world, &forward, 0.1));
    assert(body.position.y < 0.20F);
    assert(!body_penetrates(&world, &body));
}

static void test_step_down_preserves_ground(void)
{
    HTHCollisionWorld world = {0};
    HTHPlayerBody body;
    HTHPlayerMovementIntent forward = {
        {0.0F, 0.0F, -1.0F}, 1.0F, false
    };
    unsigned int step;

    world.obstacles[0] = (HTHAABB){
        {-10.0F, -1.0F, -10.0F}, {10.0F, 0.0F, 10.0F}
    };
    world.obstacles[1] = (HTHAABB){
        {-2.0F, 0.0F, -0.2F}, {2.0F, 0.20F, 2.0F}
    };
    world.obstacle_count = 2;
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.20F, 0.3F)));
    body.grounded = true;
    for (step = 0; step < 20 && body.position.y > 0.0F; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &forward, 1.0 / 60.0));
    }
    assert(close_enough(body.position.y, 0.0F));
    assert(body.grounded);
    assert(!body_penetrates(&world, &body));
}

static void test_large_drop_does_not_snap(void)
{
    HTHCollisionWorld world = {0};
    HTHPlayerBody body;
    HTHPlayerMovementIntent forward = {
        {0.0F, 0.0F, -1.0F}, 1.0F, false
    };
    unsigned int step;

    world.obstacles[0] = (HTHAABB){
        {-10.0F, -1.0F, -10.0F}, {10.0F, 0.0F, 10.0F}
    };
    world.obstacles[1] = (HTHAABB){
        {-2.0F, 0.0F, -0.2F}, {2.0F, 0.60F, 2.0F}
    };
    world.obstacle_count = 2;
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.60F, 0.3F)));
    body.grounded = true;
    for (step = 0; step < 20 && body.grounded; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &forward, 1.0 / 60.0));
    }
    assert(!body.grounded);
    assert(body.position.y > 0.30F);
    assert(!body_penetrates(&world, &body));
}

static void test_start_solid_stops_safely(void)
{
    HTHCollisionWorld world = floor_and_step_world(0.60F);
    HTHPlayerBody body;
    HTHPlayerMovementIntent forward = {
        {0.0F, 0.0F, -1.0F}, 1.0F, false
    };

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.1F, -0.7F)));
    body.grounded = false;
    assert(body_penetrates(&world, &body));
    assert(hth_player_movement_step(&body, &world, &forward, 0.1));
    assert(close_enough(body.velocity.x, 0.0F));
    assert(close_enough(body.velocity.y, 0.0F));
    assert(close_enough(body.velocity.z, 0.0F));
    assert(isfinite(body.position.x));
    assert(isfinite(body.position.y));
    assert(isfinite(body.position.z));
}

static void test_camera_eye_follows_step_up_and_down(void)
{
    HTHCollisionWorld world = floor_and_step_world(0.20F);
    HTHPlayerBody body;
    HTHPlayerMovementIntent forward = {
        {0.0F, 0.0F, -1.0F}, 1.0F, false
    };
    HTHVec3 eye;
    unsigned int step;

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = true;
    assert(hth_player_movement_step(&body, &world, &forward, 0.1));
    assert(hth_player_body_eye_position(&body, &eye));
    assert(close_enough(eye.y, 1.80F));
    for (step = 0; step < 20 && body.position.y > 0.0F; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &forward, 1.0 / 60.0));
    }
    assert(hth_player_body_eye_position(&body, &eye));
    assert(close_enough(eye.y, 1.60F));
}

static void test_jump_arc_ceiling_and_landing(void)
{
    HTHCollisionWorld world = {0};
    HTHPlayerBody body;
    HTHPlayerMovementIntent jump = {
        {0.0F, 0.0F, 0.0F}, 0.0F, true
    };
    HTHPlayerMovementIntent none = {
        {0.0F, 0.0F, 0.0F}, 0.0F, false
    };
    bool hit_ceiling = false;
    bool descended = false;
    unsigned int frame;

    world.obstacles[0] = (HTHAABB){
        {-10.0F, -1.0F, -10.0F}, {10.0F, 0.0F, 10.0F}
    };
    world.obstacles[1] = (HTHAABB){
        {-2.0F, 2.20F, -2.0F}, {2.0F, 2.50F, 2.0F}
    };
    world.obstacle_count = 2;
    assert(hth_collision_world_is_valid(&world));
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = true;
    assert(hth_player_movement_step(&body, &world, &jump, 0.01));
    assert(!body.grounded);
    assert(body.velocity.y > 0.0F);
    assert(!body_penetrates(&world, &body));

    for (frame = 0; frame < 300 && !body.grounded; ++frame) {
        float previous_y = body.velocity.y;

        assert(hth_player_movement_step(&body, &world, &none, 0.01));
        assert(!body_penetrates(&world, &body));
        if (previous_y > 0.0F && body.velocity.y == 0.0F) {
            hit_ceiling = true;
        }
        if (body.velocity.y < 0.0F) {
            descended = true;
        }
    }
    assert(hit_ceiling);
    assert(descended);
    assert(body.grounded);
    assert(close_enough(body.position.y, 0.0F));
    assert(close_enough(body.velocity.y, 0.0F));
}

static void test_jump_has_priority_and_no_double_jump(void)
{
    HTHCollisionWorld world = floor_and_step_world(0.20F);
    HTHPlayerBody body;
    HTHPlayerMovementIntent jump_forward = {
        {0.0F, 0.0F, -1.0F}, 1.0F, true
    };
    float first_vertical_velocity;

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = true;
    assert(hth_player_movement_step(
        &body, &world, &jump_forward, 0.05));
    assert(!body.grounded);
    assert(body.position.y > 0.0F);
    assert(body.position.y < 0.20F);
    first_vertical_velocity = body.velocity.y;
    assert(hth_player_movement_step(
        &body, &world, &jump_forward, 0.05));
    assert(body.velocity.y < first_vertical_velocity);
    assert(!body.grounded);
}

static void test_airborne_jump_into_wall_uses_normal_slide(void)
{
    HTHCollisionWorld world = {0};
    HTHPlayerBody body;
    HTHPlayerMovementIntent jump_right = {
        {1.0F, 0.0F, 0.0F}, 1.0F, true
    };
    HTHPlayerMovementIntent right = {
        {1.0F, 0.0F, 0.0F}, 1.0F, false
    };
    float previous_vertical_velocity;
    unsigned int frame;

    world.obstacles[0] = (HTHAABB){
        {-10.0F, -1.0F, -10.0F}, {10.0F, 0.0F, 10.0F}
    };
    world.obstacles[1] = (HTHAABB){
        {0.8F, 0.0F, -2.0F}, {1.8F, 4.0F, 2.0F}
    };
    world.obstacle_count = 2;
    assert(hth_collision_world_is_valid(&world));
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = true;
    assert(hth_player_movement_step(&body, &world, &jump_right, 0.05));
    previous_vertical_velocity = body.velocity.y;
    for (frame = 0; frame < 10; ++frame) {
        assert(hth_player_movement_step(&body, &world, &right, 0.05));
        assert(!body_penetrates(&world, &body));
    }
    assert(body.position.x <= 0.5001F);
    assert(close_enough(body.velocity.x, 0.0F));
    assert(body.velocity.y < previous_vertical_velocity);
}

static void test_movement_result_landing_speed_and_transitions(void)
{
    HTHCollisionWorld world = {0};
    HTHMovementConfig config = hth_movement_config_default();
    HTHPlayerBody body;
    HTHPlayerMovementIntent none = {
        {0.0F, 0.0F, 0.0F}, 0.0F, false
    };
    HTHPlayerMovementResult result;

    world.obstacles[0] = (HTHAABB){
        {-10.0F, -1.0F, -10.0F}, {10.0F, 0.0F, 10.0F}
    };
    world.obstacle_count = 1;
    assert(hth_collision_world_is_valid(&world));
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.30F, 0.0F)));
    body.velocity.y = -5.0F;
    assert(hth_player_movement_step_with_result(
        &body, &world, &config, &none, 0.1, &result));
    assert(result.landed);
    assert(close_enough(result.landing_speed, 5.981F));
    assert(body.grounded);
    assert(close_enough(body.velocity.y, 0.0F));

    assert(hth_player_movement_step_with_result(
        &body, &world, &config, &none, 0.1, &result));
    assert(!result.landed);
    assert(close_enough(result.landing_speed, 0.0F));

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 5.0F, 0.0F)));
    assert(hth_player_movement_step_with_result(
        &body, &world, &config, &none, 0.01, &result));
    assert(!result.landed);
}

static void test_jump_reports_one_landing(void)
{
    HTHCollisionWorld world = {0};
    HTHMovementConfig config = hth_movement_config_default();
    HTHPlayerBody body;
    HTHPlayerMovementIntent jump = {
        {0.0F, 0.0F, 0.0F}, 0.0F, true
    };
    HTHPlayerMovementIntent none = {
        {0.0F, 0.0F, 0.0F}, 0.0F, false
    };
    HTHPlayerMovementResult result;
    unsigned int landing_count = 0;
    unsigned int frame;

    world.obstacles[0] = (HTHAABB){
        {-10.0F, -1.0F, -10.0F}, {10.0F, 0.0F, 10.0F}
    };
    world.obstacle_count = 1;
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = true;
    for (frame = 0; frame < 300; ++frame) {
        assert(hth_player_movement_step_with_result(
            &body, &world, &config, frame == 0 ? &jump : &none,
            0.01, &result));
        if (result.landed) {
            landing_count++;
            assert(result.landing_speed > 0.0F);
        }
    }
    assert(landing_count == 1);
}

int main(void)
{
    test_intent();
    test_normalized_release_clears_directional_intent();
    test_repeat_restores_directional_intent_after_reconciliation();
    test_repeat_restores_diagonal_intent_after_reconciliation();
    test_jump_intent_uses_pressed_transition();
    test_integration();
    test_gravity_lands_on_floor();
    test_repeated_side_push_does_not_step_up();
    test_diagonal_push_slides_without_step_up();
    test_true_box_landing_from_above();
    test_floor_ground_stability();
    test_wall_slide_and_repeated_push();
    test_inside_corner_stability();
    test_low_exact_and_high_steps();
    test_step_requires_head_clearance();
    test_step_requires_better_progress();
    test_airborne_body_does_not_step();
    test_step_down_preserves_ground();
    test_large_drop_does_not_snap();
    test_start_solid_stops_safely();
    test_camera_eye_follows_step_up_and_down();
    test_jump_arc_ceiling_and_landing();
    test_jump_has_priority_and_no_double_jump();
    test_airborne_jump_into_wall_uses_normal_slide();
    test_movement_result_landing_speed_and_transitions();
    test_jump_reports_one_landing();
    return 0;
}
