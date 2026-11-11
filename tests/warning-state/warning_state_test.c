#include "warning_state.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return 1;                                                       \
        }                                                                   \
    } while (0)

#define CHECK_OUTPUT(state, expected_type, expected_count, expected_color) \
    do {                                                                   \
        struct lf_warning_output actual = lf_warning_get_output(&(state)); \
        CHECK(actual.type == (expected_type));                             \
        CHECK(actual.led_count == (expected_count));                       \
        CHECK(actual.color == (expected_color));                           \
    } while (0)

static struct lf_warning_config test_config(bool connection_enabled, uint8_t threshold) {
    return (struct lf_warning_config){
        .peripheral_count = 2,
        .required_count = -1,
        .device_numbers = {1, 2},
        .connection_enabled = connection_enabled,
        .battery_threshold = threshold,
        .battery_blinks = 5,
        .blink_interval_ms = 500,
        .strip_length = 4,
    };
}

static int advance_until(struct lf_warning_state *state, uint64_t limit_ms) {
    for (int steps = 0; steps < 1000; steps++) {
        uint64_t deadline = lf_warning_next_deadline(state);
        if (deadline == LF_WARNING_NO_DEADLINE || deadline > limit_ms) {
            return 0;
        }
        lf_warning_advance(state, deadline);
        CHECK(lf_warning_next_deadline(state) != deadline);
    }
    CHECK(false);
    return 1;
}

static int test_invalid_internal_config(void) {
    struct lf_warning_config config = test_config(true, 20);
    struct lf_warning_state state;
    CHECK(lf_warning_init(&state, &config, 0));
    CHECK(!lf_warning_init(&state, NULL, 0));
    CHECK(state.boot_closed);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    CHECK(lf_warning_next_deadline(&state) == LF_WARNING_NO_DEADLINE);

    config.peripheral_count = LF_WARNING_MAX_PERIPHERALS + 1;
    CHECK(!lf_warning_init(&state, &config, 0));
    config = test_config(true, 20);
    config.required_count = 3;
    CHECK(!lf_warning_init(&state, &config, 0));
    config = test_config(true, 20);
    config.device_numbers[1] = 1;
    CHECK(!lf_warning_init(&state, &config, 0));
    config = test_config(true, 20);
    config.blink_interval_ms = 0;
    CHECK(!lf_warning_init(&state, &config, 0));
    config = test_config(true, 20);
    config.battery_blinks = 0;
    CHECK(!lf_warning_init(&state, &config, 0));
    config = test_config(true, 20);
    config.battery_threshold = 1;
    CHECK(!lf_warning_init(&state, &config, 0));
    config = test_config(true, 20);
    config.strip_length = 0;
    CHECK(!lf_warning_init(&state, &config, 0));
    return 0;
}

static int test_disabled_defaults(void) {
    struct lf_warning_config config = test_config(false, 0);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    lf_warning_battery_event(&state, 10, 0, 10);
    lf_warning_connection_event(&state, 20, 0, false);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    CHECK(advance_until(&state, 20000) == 0);
    lf_warning_battery_event(&state, 20001, 1, 10);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    CHECK(state.battery_levels[1] == 10);
    return 0;
}

static int test_required_connections(void) {
    struct lf_warning_config config = test_config(true, 0);
    config.peripheral_count = 3;
    config.required_count = 2;
    config.device_numbers[0] = 2;
    config.device_numbers[1] = 1;
    config.device_numbers[2] = 3;
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 2, 0xFF0000);
    lf_warning_connection_event(&state, 10, 0, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 1, 0xFF0000);
    lf_warning_advance(&state, 500);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 0, 0);
    lf_warning_connection_event(&state, 600, 1, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    lf_warning_connection_event(&state, 700, 2, false);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);

    config.strip_length = 1;
    lf_warning_init(&state, &config, 0);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 1, 0xFF0000);
    config.required_count = 0;
    lf_warning_init(&state, &config, 0);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_battery_eligibility(void) {
    struct lf_warning_config config = {
        .peripheral_count = 3,
        .required_count = 2,
        .device_numbers = {2, 1, 3},
        .battery_threshold = 20,
        .battery_blinks = 5,
        .blink_interval_ms = 500,
        .strip_length = 4,
    };
    struct lf_warning_state state;

    lf_warning_init(&state, &config, 1000);
    lf_warning_battery_event(&state, 2000, 0, 0);
    lf_warning_battery_event(&state, 2000, 2, 10);
    CHECK(state.battery_status == LF_WARNING_BATTERY_NONE);
    lf_warning_battery_event(&state, 20999, 1, 10);
    CHECK(state.battery_status == LF_WARNING_BATTERY_ACTIVE);

    lf_warning_init(&state, &config, 1000);
    lf_warning_battery_event(&state, 21000, 1, 10);
    CHECK(state.boot_closed);
    CHECK(state.battery_status == LF_WARNING_BATTERY_NONE);
    CHECK(state.battery_levels[1] == 10);
    return 0;
}

static int test_battery_sequence(void) {
    struct lf_warning_config config = test_config(false, 20);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_battery_event(&state, 100, 0, 10);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 4, 0xFF0000);
    lf_warning_advance(&state, 600);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 4, 0xFFFF00);
    lf_warning_advance(&state, 1100);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 0, 0);
    lf_warning_advance(&state, 2100);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 4, 0xFF0000);
    CHECK(advance_until(&state, 9099) == 0);
    CHECK(state.battery_status == LF_WARNING_BATTERY_ACTIVE);
    CHECK(advance_until(&state, 9100) == 0);
    CHECK(state.battery_status == LF_WARNING_BATTERY_DONE);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    lf_warning_battery_event(&state, 9200, 1, 5);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_connection_priority(void) {
    struct lf_warning_config config = test_config(true, 20);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_connection_event(&state, 100, 0, true);
    lf_warning_battery_event(&state, 200, 0, 10);
    CHECK(state.battery_status == LF_WARNING_BATTERY_PENDING);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 2, 0xFF0000);
    lf_warning_battery_event(&state, 300, 0, 80);
    lf_warning_connection_event(&state, 400, 1, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 4, 0xFF0000);
    CHECK(advance_until(&state, 9400) == 0);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_unresolved_connection(void) {
    struct lf_warning_config config = test_config(true, 20);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_connection_event(&state, 10, 0, true);
    lf_warning_battery_event(&state, 20, 0, 10);
    CHECK(advance_until(&state, 19999) == 0);
    CHECK(state.connection_active);
    CHECK(state.battery_status == LF_WARNING_BATTERY_PENDING);
    lf_warning_advance(&state, 20000);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 4, 0xFF0000);
    CHECK(state.boot_closed);
    CHECK(advance_until(&state, 29000) == 0);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_boot_cutoff(void) {
    struct lf_warning_config config = test_config(false, 20);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 1000);
    lf_warning_battery_event(&state, 20999, 0, 10);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 4, 0xFF0000);
    CHECK(advance_until(&state, 29999) == 0);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);

    lf_warning_init(&state, &config, 1000);
    lf_warning_battery_event(&state, 21000, 0, 10);
    CHECK(state.boot_closed);
    CHECK(state.battery_levels[0] == 10);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    lf_warning_battery_event(&state, 21001, 1, 5);
    CHECK(state.battery_levels[1] == 5);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_late_connection_minimum(void) {
    struct lf_warning_config config = test_config(true, 0);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_connection_event(&state, 0, 0, true);
    lf_warning_connection_event(&state, 0, 1, true);
    lf_warning_connection_event(&state, 19900, 0, false);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 1, 0xFF0000);
    lf_warning_advance(&state, 20000);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 1, 0xFF0000);
    lf_warning_advance(&state, 20400);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 0, 0);
    lf_warning_advance(&state, 20900);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 1, 0xFF0000);
    lf_warning_advance(&state, 21400);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);

    lf_warning_init(&state, &config, 0);
    lf_warning_connection_event(&state, 0, 0, true);
    lf_warning_connection_event(&state, 0, 1, true);
    lf_warning_connection_event(&state, 19900, 0, false);
    lf_warning_connection_event(&state, 20001, 0, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_delayed_deadline(void) {
    struct lf_warning_config config = test_config(true, 0);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_connection_event(&state, 0, 0, true);
    lf_warning_connection_event(&state, 0, 1, true);
    lf_warning_connection_event(&state, 19900, 0, false);
    lf_warning_connection_event(&state, 20001, 1, false);
    CHECK(state.boot_closed);
    CHECK(state.eligible_missing_slots == 1);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 1, 0xFF0000);
    lf_warning_connection_event(&state, 20002, 0, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    CHECK(state.connected_slots == 1);

    lf_warning_init(&state, &config, 0);
    lf_warning_connection_event(&state, 0, 0, true);
    lf_warning_connection_event(&state, 0, 1, true);
    lf_warning_connection_event(&state, 20001, 1, false);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    CHECK(state.connected_slots == 1);
    return 0;
}

static int test_late_second_connection(void) {
    struct lf_warning_config config = test_config(true, 0);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_connection_event(&state, 0, 0, true);
    CHECK(advance_until(&state, 18999) == 0);
    uint8_t completed = state.connection_blinks_completed;
    CHECK(completed >= 2);
    lf_warning_connection_event(&state, 19000, 0, false);
    CHECK(state.connection_blinks_completed >= completed);
    CHECK(advance_until(&state, 20000) == 0);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_post_deadline_reconnects(void) {
    struct lf_warning_config config = test_config(true, 0);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_connection_event(&state, 0, 0, true);
    lf_warning_connection_event(&state, 0, 1, true);
    lf_warning_connection_event(&state, 19900, 0, false);
    lf_warning_connection_event(&state, 19901, 1, false);
    lf_warning_advance(&state, 20000);
    CHECK(state.eligible_missing_slots == 3);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 2, 0xFF0000);
    lf_warning_connection_event(&state, 20001, 1, true);
    CHECK(state.eligible_missing_slots == 1);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 1, 0xFF0000);
    lf_warning_connection_event(&state, 20002, 1, false);
    CHECK(state.eligible_missing_slots == 1);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 1, 0xFF0000);
    lf_warning_connection_event(&state, 20003, 0, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    CHECK(state.connected_slots == 1);
    return 0;
}

static int test_delayed_blink_work(void) {
    struct lf_warning_config config = test_config(true, 0);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_advance(&state, 21000);
    CHECK(state.boot_closed);
    CHECK(state.connection_blinks_completed == 1);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 0, 0);
    lf_warning_advance(&state, 21500);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 2, 0xFF0000);
    lf_warning_advance(&state, 22000);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_battery_restart_resolved(void) {
    struct lf_warning_config config = test_config(true, 20);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_connection_event(&state, 0, 0, true);
    lf_warning_connection_event(&state, 0, 1, true);
    lf_warning_battery_event(&state, 1000, 0, 10);
    lf_warning_advance(&state, 1500);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 4, 0xFFFF00);
    lf_warning_connection_event(&state, 1600, 1, false);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 2, 0xFF0000);
    lf_warning_connection_event(&state, 1700, 1, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 4, 0xFF0000);
    CHECK(state.battery_blinks_completed == 0);
    CHECK(advance_until(&state, 10700) == 0);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_battery_restart_unresolved(void) {
    struct lf_warning_config config = test_config(true, 20);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_connection_event(&state, 0, 0, true);
    lf_warning_connection_event(&state, 0, 1, true);
    lf_warning_battery_event(&state, 1000, 0, 10);
    lf_warning_connection_event(&state, 1600, 1, false);
    CHECK(advance_until(&state, 19999) == 0);
    lf_warning_advance(&state, 20000);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 4, 0xFF0000);
    CHECK(state.battery_blinks_completed == 0);
    CHECK(advance_until(&state, 29000) == 0);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_display_cancellation(void) {
    struct lf_warning_config config = test_config(true, 20);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_connection_event(&state, 0, 0, true);
    lf_warning_connection_event(&state, 0, 1, true);
    lf_warning_battery_event(&state, 100, 0, 10);
    lf_warning_set_display(&state, 200, false);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    CHECK(state.battery_status == LF_WARNING_BATTERY_DONE);
    lf_warning_set_display(&state, 300, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);

    lf_warning_init(&state, &config, 0);
    lf_warning_connection_event(&state, 0, 0, true);
    lf_warning_connection_event(&state, 0, 1, true);
    lf_warning_battery_event(&state, 100, 0, 10);
    lf_warning_connection_event(&state, 200, 1, false);
    CHECK(state.battery_status == LF_WARNING_BATTERY_PENDING);
    lf_warning_set_display(&state, 300, false);
    CHECK(state.battery_status == LF_WARNING_BATTERY_DONE);
    lf_warning_set_display(&state, 400, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 2, 0xFF0000);
    lf_warning_connection_event(&state, 500, 1, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_display_pending(void) {
    struct lf_warning_config config = test_config(false, 20);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_set_display(&state, 100, false);
    lf_warning_battery_event(&state, 200, 0, 10);
    CHECK(state.battery_status == LF_WARNING_BATTERY_PENDING);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    lf_warning_set_display(&state, 300, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 4, 0xFF0000);

    lf_warning_init(&state, &config, 0);
    lf_warning_set_display(&state, 100, false);
    lf_warning_battery_event(&state, 200, 0, 10);
    lf_warning_advance(&state, 20000);
    CHECK(state.battery_status == LF_WARNING_BATTERY_DONE);
    lf_warning_set_display(&state, 20001, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_display_pending_connection(void) {
    struct lf_warning_config config = test_config(true, 20);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_set_display(&state, 100, false);
    lf_warning_connection_event(&state, 200, 0, true);
    lf_warning_battery_event(&state, 300, 0, 10);
    CHECK(state.battery_status == LF_WARNING_BATTERY_PENDING);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    lf_warning_set_display(&state, 400, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 2, 0xFF0000);
    lf_warning_connection_event(&state, 500, 1, true);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 4, 0xFF0000);
    CHECK(advance_until(&state, 9500) == 0);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_independent_options(void) {
    struct lf_warning_config config = test_config(false, 20);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_battery_event(&state, 100, 0, 10);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 4, 0xFF0000);

    config = test_config(true, 0);
    lf_warning_init(&state, &config, 0);
    lf_warning_battery_event(&state, 100, 0, 10);
    CHECK(state.battery_status == LF_WARNING_BATTERY_NONE);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_CONNECTION, 2, 0xFF0000);

    config.required_count = 0;
    config.battery_threshold = 20;
    lf_warning_init(&state, &config, 0);
    lf_warning_battery_event(&state, 100, 0, 10);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_post_boot_updates(void) {
    struct lf_warning_config config = test_config(true, 20);
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_connection_event(&state, 0, 0, true);
    lf_warning_connection_event(&state, 0, 1, true);
    lf_warning_advance(&state, 20000);
    lf_warning_connection_event(&state, 20001, 1, false);
    lf_warning_battery_event(&state, 20002, 0, 10);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    CHECK(state.battery_levels[0] == 10);
    CHECK(state.connected_slots == 1);
    lf_warning_connection_event(&state, 20003, 1, true);
    CHECK(state.connected_slots == 3);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    return 0;
}

static int test_maximum_battery_sequence(void) {
    struct lf_warning_config config = test_config(false, 100);
    config.battery_blinks = 10;
    config.blink_interval_ms = 1000;
    config.strip_length = 6;
    struct lf_warning_state state;
    lf_warning_init(&state, &config, 0);
    lf_warning_battery_event(&state, 19999, 0, 10);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_BATTERY, 4, 0xFF0000);
    CHECK(advance_until(&state, 48998) == 0);
    CHECK(state.battery_status == LF_WARNING_BATTERY_ACTIVE);
    CHECK(advance_until(&state, 48999) == 0);
    CHECK_OUTPUT(state, LF_WARNING_OUTPUT_NONE, 0, 0);
    CHECK(lf_warning_next_deadline(&state) == LF_WARNING_NO_DEADLINE);
    return 0;
}

struct test_case {
    const char *name;
    int (*run)(void);
};

int main(int argc, char **argv) {
    static const struct test_case cases[] = {
        {"invalid_internal_config", test_invalid_internal_config},
        {"disabled_defaults", test_disabled_defaults},
        {"required_connections", test_required_connections},
        {"battery_eligibility", test_battery_eligibility},
        {"battery_sequence", test_battery_sequence},
        {"connection_priority", test_connection_priority},
        {"unresolved_connection", test_unresolved_connection},
        {"boot_cutoff", test_boot_cutoff},
        {"late_connection_minimum", test_late_connection_minimum},
        {"late_second_connection", test_late_second_connection},
        {"delayed_deadline", test_delayed_deadline},
        {"post_deadline_reconnects", test_post_deadline_reconnects},
        {"delayed_blink_work", test_delayed_blink_work},
        {"battery_restart_resolved", test_battery_restart_resolved},
        {"battery_restart_unresolved", test_battery_restart_unresolved},
        {"display_cancellation", test_display_cancellation},
        {"display_pending", test_display_pending},
        {"display_pending_connection", test_display_pending_connection},
        {"independent_options", test_independent_options},
        {"post_boot_updates", test_post_boot_updates},
        {"maximum_battery_sequence", test_maximum_battery_sequence},
    };
    if (argc != 2) {
        fprintf(stderr, "Expected one test case name\n");
        return 2;
    }
    for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); index++) {
        if (strcmp(argv[1], cases[index].name) == 0) {
            return cases[index].run();
        }
    }
    fprintf(stderr, "Unknown test case: %s\n", argv[1]);
    return 2;
}