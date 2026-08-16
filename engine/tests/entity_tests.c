#include "entity.h"

#include <stdbool.h>
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

static bool handle_is_invalid(HTHEntityHandle handle)
{
    return hth_entity_handle_equal(handle, hth_entity_handle_invalid());
}

static bool test_registry_lifecycle_and_null_arguments(void)
{
    HTHEntityRegistry *registry = hth_entity_registry_create();
    HTHEntityHandle output = {2U, 3U};
    HTHEntityIterator iterator = {9U};

    CHECK(registry != NULL);
    CHECK(hth_entity_registry_live_count(registry) == 0U);
    CHECK(hth_entity_registry_live_count(NULL) == 0U);
    CHECK(hth_entity_registry_test_validate(registry));
    CHECK(!hth_entity_registry_create_entity(NULL, &output));
    CHECK(handle_is_invalid(output));
    CHECK(!hth_entity_registry_create_entity(registry, NULL));
    CHECK(!hth_entity_registry_destroy_entity(
        NULL, hth_entity_handle_invalid()));
    CHECK(!hth_entity_registry_is_alive(
        NULL, hth_entity_handle_invalid()));
    hth_entity_iterator_begin(NULL);
    CHECK(!hth_entity_iterator_next(NULL, &iterator, &output));
    CHECK(handle_is_invalid(output));
    CHECK(!hth_entity_iterator_next(registry, NULL, &output));
    CHECK(handle_is_invalid(output));
    CHECK(!hth_entity_iterator_next(registry, &iterator, NULL));
    hth_entity_registry_destroy(registry);
    hth_entity_registry_destroy(NULL);
    return true;
}

static bool test_fresh_entities_and_equality(void)
{
    HTHEntityRegistry *registry = hth_entity_registry_create();
    HTHEntityHandle handles[10];
    HTHEntityHandle altered;
    size_t index;

    CHECK(registry != NULL);
    for (index = 0U; index < 10U; ++index) {
        CHECK(hth_entity_registry_create_entity(registry, &handles[index]));
        CHECK(handles[index].index == (uint32_t)index);
        CHECK(handles[index].generation == 1U);
        CHECK(hth_entity_registry_is_alive(registry, handles[index]));
        CHECK(hth_entity_registry_live_count(registry) == index + 1U);
    }
    CHECK(hth_entity_handle_equal(handles[4], handles[4]));
    CHECK(hth_entity_handle_equal(hth_entity_handle_invalid(),
                                  hth_entity_handle_invalid()));
    CHECK(!hth_entity_handle_equal(handles[4], handles[5]));
    altered = handles[4];
    altered.generation++;
    CHECK(!hth_entity_handle_equal(handles[4], altered));
    CHECK(hth_entity_registry_test_validate(registry));
    hth_entity_registry_destroy(registry);
    return true;
}

static bool test_destroy_reuse_and_stale_safety(void)
{
    HTHEntityRegistry *registry = hth_entity_registry_create();
    HTHEntityHandle first;
    HTHEntityHandle second;
    HTHEntityHandle wrong;
    HTHEntityHandle iterated;
    HTHEntityIterator iterator;

    CHECK(registry != NULL);
    CHECK(hth_entity_registry_create_entity(registry, &first));
    CHECK(hth_entity_registry_destroy_entity(registry, first));
    CHECK(!hth_entity_registry_is_alive(registry, first));
    CHECK(hth_entity_registry_live_count(registry) == 0U);
    CHECK(!hth_entity_registry_destroy_entity(registry, first));
    CHECK(hth_entity_registry_create_entity(registry, &second));
    CHECK(second.index == first.index);
    CHECK(second.generation == first.generation + 1U);
    CHECK(!hth_entity_registry_destroy_entity(registry, first));
    CHECK(hth_entity_registry_is_alive(registry, second));
    hth_entity_iterator_begin(&iterator);
    CHECK(hth_entity_iterator_next(registry, &iterator, &iterated));
    CHECK(hth_entity_handle_equal(iterated, second));
    CHECK(!hth_entity_iterator_next(registry, &iterator, &iterated));
    wrong = second;
    wrong.generation++;
    CHECK(!hth_entity_registry_destroy_entity(registry, wrong));
    CHECK(hth_entity_registry_is_alive(registry, second));
    CHECK(hth_entity_registry_live_count(registry) == 1U);
    CHECK(hth_entity_registry_test_validate(registry));
    hth_entity_registry_destroy(registry);
    return true;
}

static bool test_invalid_handles_do_not_mutate(void)
{
    HTHEntityRegistry *registry = hth_entity_registry_create();
    HTHEntityHandle live;
    HTHEntityHandle invalid = hth_entity_handle_invalid();
    HTHEntityHandle zero_generation = {0U, 0U};
    HTHEntityHandle out_of_range = {1000U, 1U};
    HTHEntityIterator iterator;
    HTHEntityHandle iterated;

    CHECK(registry != NULL);
    CHECK(hth_entity_registry_create_entity(registry, &live));
    CHECK(!hth_entity_registry_destroy_entity(registry, invalid));
    CHECK(!hth_entity_registry_destroy_entity(registry, zero_generation));
    CHECK(!hth_entity_registry_destroy_entity(registry, out_of_range));
    CHECK(!hth_entity_registry_is_alive(registry, invalid));
    CHECK(!hth_entity_registry_is_alive(registry, zero_generation));
    CHECK(!hth_entity_registry_is_alive(registry, out_of_range));
    CHECK(hth_entity_registry_live_count(registry) == 1U);
    hth_entity_iterator_begin(&iterator);
    CHECK(hth_entity_iterator_next(registry, &iterator, &iterated));
    CHECK(hth_entity_handle_equal(iterated, live));
    CHECK(!hth_entity_iterator_next(registry, &iterator, &iterated));
    CHECK(handle_is_invalid(iterated));
    CHECK(hth_entity_registry_test_validate(registry));
    hth_entity_registry_destroy(registry);
    return true;
}

static bool test_growth_and_handle_stability(void)
{
    enum { ENTITY_COUNT = 129 };
    HTHEntityRegistry *registry = hth_entity_registry_create();
    HTHEntityHandle handles[ENTITY_COUNT];
    HTHEntityHandle iterated;
    HTHEntityIterator iterator;
    size_t iterated_count = 0U;
    size_t index;

    CHECK(registry != NULL);
    for (index = 0U; index < ENTITY_COUNT; ++index) {
        CHECK(hth_entity_registry_create_entity(registry, &handles[index]));
        CHECK(handles[index].index == (uint32_t)index);
        CHECK(handles[index].generation == 1U);
    }
    CHECK(handles[64].index == 64U);
    CHECK(handles[128].index == 128U);
    CHECK(hth_entity_registry_live_count(registry) == ENTITY_COUNT);
    for (index = 0U; index < ENTITY_COUNT; ++index) {
        CHECK(hth_entity_registry_is_alive(registry, handles[index]));
    }
    hth_entity_iterator_begin(&iterator);
    while (hth_entity_iterator_next(registry, &iterator, &iterated)) {
        CHECK(hth_entity_registry_is_alive(registry, iterated));
        iterated_count++;
    }
    CHECK(iterated_count == ENTITY_COUNT);
    CHECK(iterated_count == hth_entity_registry_live_count(registry));
    CHECK(hth_entity_registry_test_validate(registry));
    hth_entity_registry_destroy(registry);
    return true;
}

static bool test_mass_destroy_and_lifo_reuse(void)
{
    enum { ENTITY_COUNT = 200 };
    HTHEntityRegistry *registry = hth_entity_registry_create();
    HTHEntityHandle handles[ENTITY_COUNT];
    HTHEntityHandle reused;
    size_t index;

    CHECK(registry != NULL);
    for (index = 0U; index < ENTITY_COUNT; ++index) {
        CHECK(hth_entity_registry_create_entity(registry, &handles[index]));
    }
    for (index = 0U; index < ENTITY_COUNT; index += 2U) {
        CHECK(hth_entity_registry_destroy_entity(registry, handles[index]));
    }
    CHECK(hth_entity_registry_live_count(registry) == ENTITY_COUNT / 2U);
    for (index = 0U; index < ENTITY_COUNT; ++index) {
        CHECK(hth_entity_registry_is_alive(registry, handles[index]) ==
              (index % 2U != 0U));
    }
    for (index = ENTITY_COUNT; index > 0U; index -= 2U) {
        size_t expected = index - 2U;

        CHECK(hth_entity_registry_create_entity(registry, &reused));
        CHECK(reused.index == (uint32_t)expected);
        CHECK(reused.generation == 2U);
    }
    CHECK(hth_entity_registry_live_count(registry) == ENTITY_COUNT);
    for (index = 0U; index < ENTITY_COUNT; ++index) {
        if (index % 2U == 0U) {
            CHECK(!hth_entity_registry_is_alive(registry, handles[index]));
            CHECK(!hth_entity_registry_destroy_entity(registry,
                                                       handles[index]));
        } else {
            CHECK(hth_entity_registry_is_alive(registry, handles[index]));
        }
    }
    CHECK(hth_entity_registry_live_count(registry) == ENTITY_COUNT);
    CHECK(hth_entity_registry_test_validate(registry));
    hth_entity_registry_destroy(registry);
    return true;
}

static bool test_iteration_empty_full_holes_and_exhaustion(void)
{
    HTHEntityRegistry *registry = hth_entity_registry_create();
    HTHEntityHandle handles[8];
    HTHEntityHandle output = {7U, 7U};
    HTHEntityIterator iterator;
    size_t expected;

    CHECK(registry != NULL);
    hth_entity_iterator_begin(&iterator);
    CHECK(!hth_entity_iterator_next(registry, &iterator, &output));
    CHECK(handle_is_invalid(output));
    for (expected = 0U; expected < 8U; ++expected) {
        CHECK(hth_entity_registry_create_entity(registry, &handles[expected]));
    }
    hth_entity_iterator_begin(&iterator);
    for (expected = 0U; expected < 8U; ++expected) {
        CHECK(hth_entity_iterator_next(registry, &iterator, &output));
        CHECK(hth_entity_handle_equal(output, handles[expected]));
    }
    CHECK(!hth_entity_iterator_next(registry, &iterator, &output));
    CHECK(handle_is_invalid(output));
    CHECK(!hth_entity_iterator_next(registry, &iterator, &output));
    CHECK(handle_is_invalid(output));

    CHECK(hth_entity_registry_destroy_entity(registry, handles[1]));
    CHECK(hth_entity_registry_destroy_entity(registry, handles[4]));
    CHECK(hth_entity_registry_destroy_entity(registry, handles[6]));
    hth_entity_iterator_begin(&iterator);
    expected = 0U;
    while (hth_entity_iterator_next(registry, &iterator, &output)) {
        static const uint32_t expected_indices[] = {0U, 2U, 3U, 5U, 7U};

        CHECK(expected < sizeof(expected_indices) /
                         sizeof(expected_indices[0]));
        CHECK(output.index == expected_indices[expected]);
        CHECK(hth_entity_registry_is_alive(registry, output));
        expected++;
    }
    CHECK(expected == hth_entity_registry_live_count(registry));
    CHECK(handle_is_invalid(output));
    CHECK(hth_entity_registry_test_validate(registry));
    hth_entity_registry_destroy(registry);
    return true;
}

static bool test_generation_retirement(void)
{
    HTHEntityRegistry *registry = hth_entity_registry_create();
    HTHEntityHandle original;
    HTHEntityHandle final_generation;
    HTHEntityHandle next;
    HTHEntityIterator iterator;
    HTHEntityHandle iterated;

    CHECK(registry != NULL);
    CHECK(hth_entity_registry_create_entity(registry, &original));
    CHECK(hth_entity_registry_test_set_generation(
        registry, original, UINT32_MAX, &final_generation));
    CHECK(final_generation.index == 0U);
    CHECK(final_generation.generation == UINT32_MAX);
    CHECK(!hth_entity_registry_is_alive(registry, original));
    CHECK(hth_entity_registry_is_alive(registry, final_generation));
    CHECK(hth_entity_registry_destroy_entity(registry, final_generation));
    CHECK(hth_entity_registry_live_count(registry) == 0U);
    CHECK(!hth_entity_registry_is_alive(registry, final_generation));
    CHECK(!hth_entity_registry_destroy_entity(registry, final_generation));
    hth_entity_iterator_begin(&iterator);
    CHECK(!hth_entity_iterator_next(registry, &iterator, &iterated));
    CHECK(handle_is_invalid(iterated));
    CHECK(hth_entity_registry_create_entity(registry, &next));
    CHECK(next.index == 1U);
    CHECK(next.generation == 1U);
    CHECK(hth_entity_registry_test_validate(registry));
    hth_entity_registry_destroy(registry);
    return true;
}

static bool test_generation_helper_rejects_invalid_requests(void)
{
    HTHEntityRegistry *registry = hth_entity_registry_create();
    HTHEntityHandle live;
    HTHEntityHandle output = {1U, 1U};
    HTHEntityHandle stale;

    CHECK(registry != NULL);
    CHECK(hth_entity_registry_create_entity(registry, &live));
    CHECK(!hth_entity_registry_test_set_generation(
        registry, live, 0U, &output));
    CHECK(handle_is_invalid(output));
    stale = live;
    stale.generation++;
    CHECK(!hth_entity_registry_test_set_generation(
        registry, stale, UINT32_MAX, &output));
    CHECK(handle_is_invalid(output));
    CHECK(!hth_entity_registry_test_set_generation(
        NULL, live, UINT32_MAX, &output));
    CHECK(handle_is_invalid(output));
    CHECK(!hth_entity_registry_test_set_generation(
        registry, live, UINT32_MAX, NULL));
    CHECK(hth_entity_registry_is_alive(registry, live));
    CHECK(hth_entity_registry_test_validate(registry));
    hth_entity_registry_destroy(registry);
    return true;
}

static bool test_multiple_registries_are_independent(void)
{
    HTHEntityRegistry *first = hth_entity_registry_create();
    HTHEntityRegistry *second = hth_entity_registry_create();
    HTHEntityHandle first_handle;
    HTHEntityHandle second_handle;

    CHECK(first != NULL && second != NULL);
    CHECK(hth_entity_registry_create_entity(first, &first_handle));
    CHECK(hth_entity_registry_create_entity(second, &second_handle));
    CHECK(hth_entity_handle_equal(first_handle, second_handle));
    CHECK(hth_entity_registry_destroy_entity(first, first_handle));
    CHECK(hth_entity_registry_live_count(first) == 0U);
    CHECK(hth_entity_registry_live_count(second) == 1U);
    CHECK(hth_entity_registry_is_alive(second, second_handle));
    hth_entity_registry_destroy(first);
    CHECK(hth_entity_registry_is_alive(second, second_handle));
    CHECK(hth_entity_registry_test_validate(second));
    hth_entity_registry_destroy(second);
    return true;
}

static bool test_destroy_registry_with_live_entities(void)
{
    HTHEntityRegistry *registry = hth_entity_registry_create();
    HTHEntityHandle handle;
    size_t index;

    CHECK(registry != NULL);
    for (index = 0U; index < 100U; ++index) {
        CHECK(hth_entity_registry_create_entity(registry, &handle));
    }
    CHECK(hth_entity_registry_live_count(registry) == 100U);
    hth_entity_registry_destroy(registry);
    return true;
}

static bool test_thousands_of_reuse_cycles(void)
{
    HTHEntityRegistry *registry = hth_entity_registry_create();
    HTHEntityHandle current;
    HTHEntityHandle stale;
    uint32_t expected_generation = 1U;
    size_t cycle;

    CHECK(registry != NULL);
    CHECK(hth_entity_registry_create_entity(registry, &current));
    for (cycle = 0U; cycle < 5000U; ++cycle) {
        CHECK(current.index == 0U);
        CHECK(current.generation == expected_generation);
        stale = current;
        CHECK(hth_entity_registry_destroy_entity(registry, current));
        CHECK(!hth_entity_registry_is_alive(registry, stale));
        expected_generation++;
        CHECK(hth_entity_registry_create_entity(registry, &current));
        CHECK(!hth_entity_registry_destroy_entity(registry, stale));
        CHECK(hth_entity_registry_is_alive(registry, current));
    }
    CHECK(hth_entity_registry_live_count(registry) == 1U);
    CHECK(hth_entity_registry_test_validate(registry));
    hth_entity_registry_destroy(registry);
    return true;
}

static bool test_large_live_set_has_unique_valid_handles(void)
{
    enum { ENTITY_COUNT = 2048 };
    HTHEntityRegistry *registry = hth_entity_registry_create();
    HTHEntityHandle *handles = malloc(ENTITY_COUNT * sizeof(*handles));
    size_t left;
    size_t right;

    if (registry == NULL || handles == NULL) {
        hth_entity_registry_destroy(registry);
        free(handles);
        return false;
    }
    for (left = 0U; left < ENTITY_COUNT; ++left) {
        CHECK(hth_entity_registry_create_entity(registry, &handles[left]));
        CHECK(handles[left].index != UINT32_MAX);
        CHECK(handles[left].generation != 0U);
    }
    for (left = 0U; left < ENTITY_COUNT; ++left) {
        for (right = left + 1U; right < ENTITY_COUNT; ++right) {
            CHECK(!hth_entity_handle_equal(handles[left], handles[right]));
        }
    }
    CHECK(hth_entity_registry_test_validate(registry));
    free(handles);
    hth_entity_registry_destroy(registry);
    return true;
}

int main(void)
{
    if (!test_registry_lifecycle_and_null_arguments() ||
        !test_fresh_entities_and_equality() ||
        !test_destroy_reuse_and_stale_safety() ||
        !test_invalid_handles_do_not_mutate() ||
        !test_growth_and_handle_stability() ||
        !test_mass_destroy_and_lifo_reuse() ||
        !test_iteration_empty_full_holes_and_exhaustion() ||
        !test_generation_retirement() ||
        !test_generation_helper_rejects_invalid_requests() ||
        !test_multiple_registries_are_independent() ||
        !test_destroy_registry_with_live_entities() ||
        !test_thousands_of_reuse_cycles() ||
        !test_large_live_set_has_unique_valid_handles()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
