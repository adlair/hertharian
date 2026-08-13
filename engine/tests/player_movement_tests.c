#include "event.h"
#include "input_internal.h"
#include "player_movement.h"

#include <assert.h>
#include <math.h>

static bool close_enough(float left, float right)
{
    return fabsf(left - right) <= 1.0e-4F;
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

static void inject_key(HTHInput *input, HTHKey key)
{
    HTHPlatformEvent event = {0};
    event.type = HTH_PLATFORM_EVENT_KEY_DOWN;
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
    assert(diagonal.direction.x > 0.0F && diagonal.direction.z < 0.0F);
    assert(close_enough(pitched.direction.y, 0.0F));
    assert(close_enough(pitched.direction.z, -1.0F));
    assert(close_enough(disabled.direction.x, 0.0F));
    assert(close_enough(disabled.direction.z, 0.0F));
}

static void test_integration(void)
{
    HTHCollisionWorld world;
    HTHPlayerBody body;
    HTHPlayerMovementIntent forward = {{0.0F, 0.0F, -1.0F}};
    HTHPlayerMovementIntent none = {{0.0F, 0.0F, 0.0F}};

    assert(hth_collision_world_init_bootstrap(&world));
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 5.0F, 3.0F)));
    assert(hth_player_movement_step(&body, &world, &forward, 0.05));
    assert(close_enough(body.position.z, 2.8F));
    assert(close_enough(body.velocity.z, -4.0F));
    assert(close_enough(body.velocity.y, -0.4905F));
    assert(close_enough(body.position.y, 4.975475F));
    assert(!body.grounded);

    body.position = hth_vec3(0.0F, 5.0F, 3.0F);
    body.velocity = hth_vec3(0.0F, 0.0F, 0.0F);
    assert(hth_player_movement_step(&body, &world, &forward, 1.0));
    assert(close_enough(body.position.z, 2.6F));
    assert(close_enough(body.velocity.y, -0.981F));
    assert(close_enough(body.position.y, 4.9019F));

    assert(hth_player_movement_step(&body, &world, &none, 0.0));
    assert(close_enough(body.velocity.x, 0.0F));
    assert(close_enough(body.velocity.z, 0.0F));
}

static void test_gravity_lands_on_floor(void)
{
    HTHCollisionWorld world;
    HTHPlayerBody body;
    HTHPlayerMovementIntent none = {{0.0F, 0.0F, 0.0F}};
    unsigned int step;

    assert(hth_collision_world_init_bootstrap(&world));
    assert(hth_player_body_init(&body, hth_vec3(0.0F, 1.0F, 3.0F)));
    for (step = 0; step < 120 && !body.grounded; ++step) {
        assert(hth_player_movement_step(&body, &world, &none, 1.0 / 60.0));
    }
    assert(body.grounded);
    assert(close_enough(body.position.y, 0.0F));
    assert(close_enough(body.velocity.y, 0.0F));
    assert(!hth_collision_world_body_penetrates(&world, &body));
    assert(hth_player_movement_step(&body, &world, &none, 1.0 / 60.0));
    assert(body.grounded);
    assert(close_enough(body.position.y, 0.0F));
}

static void test_repeated_side_push_does_not_step_up(void)
{
    HTHCollisionWorld world = floor_and_box_world();
    HTHPlayerBody body;
    HTHPlayerMovementIntent forward = {{0.0F, 0.0F, -1.0F}};
    unsigned int step;

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = true;
    for (step = 0; step < 240; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &forward, 1.0 / 60.0));
        assert(close_enough(body.position.y, 0.0F));
        assert(!hth_collision_world_body_penetrates(&world, &body));
    }
    assert(close_enough(body.position.z, -0.7F));
    assert(body.grounded);
}

static void test_diagonal_push_slides_without_step_up(void)
{
    HTHCollisionWorld world = floor_and_box_world();
    HTHPlayerBody body;
    HTHPlayerMovementIntent diagonal = {{0.70710677F, 0.0F, -0.70710677F}};
    unsigned int step;

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 0.0F, 0.0F)));
    body.grounded = true;
    for (step = 0; step < 120; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &diagonal, 1.0 / 60.0));
        assert(close_enough(body.position.y, 0.0F));
        assert(!hth_collision_world_body_penetrates(&world, &body));
    }
    assert(body.position.x > 1.3F);
    assert(body.position.z < -0.7F);
}

static void test_true_box_landing_from_above(void)
{
    HTHCollisionWorld world = floor_and_box_world();
    HTHPlayerBody body;
    HTHPlayerMovementIntent none = {{0.0F, 0.0F, 0.0F}};
    unsigned int step;

    assert(hth_player_body_init(&body, hth_vec3(0.0F, 3.0F, -1.5F)));
    for (step = 0; step < 240 && !body.grounded; ++step) {
        assert(hth_player_movement_step(
            &body, &world, &none, 1.0 / 60.0));
    }
    assert(body.grounded);
    assert(close_enough(body.position.y, 1.0F));
    assert(close_enough(body.velocity.y, 0.0F));
    assert(!hth_collision_world_body_penetrates(&world, &body));
}

static void test_floor_ground_stability(void)
{
    HTHCollisionWorld world = floor_and_box_world();
    HTHPlayerBody body;
    HTHPlayerMovementIntent none = {{0.0F, 0.0F, 0.0F}};
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

int main(void)
{
    test_intent();
    test_integration();
    test_gravity_lands_on_floor();
    test_repeated_side_push_does_not_step_up();
    test_diagonal_push_slides_without_step_up();
    test_true_box_landing_from_above();
    test_floor_ground_stability();
    return 0;
}
