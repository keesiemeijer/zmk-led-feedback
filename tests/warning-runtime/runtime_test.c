#include <stdio.h>
#include <string.h>

#include "fake_zephyr.h"

uint64_t fake_now_ms;
struct device fake_device = {.ready = true};
struct led_rgb fake_pixels[4];
int fake_layer;

#include "../../src/led_feedback.c"

#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return 1;                                                       \
        }                                                                   \
    } while (0)

static int run_until(uint64_t limit_ms) {
    struct k_work_delayable *works[] = {
        &warning_work,
        &layer_show_work,
        &layer_clear_work,
        &battery_work,
    };
    for (int steps = 0; steps < 1000; steps++) {
        struct k_work_delayable *next = NULL;
        for (size_t index = 0; index < sizeof(works) / sizeof(works[0]); index++) {
            if (works[index]->pending && works[index]->deadline <= limit_ms &&
                (!next || works[index]->deadline < next->deadline)) {
                next = works[index];
            }
        }
        if (!next) {
            fake_now_ms = limit_ms;
            return 0;
        }
        fake_now_ms = next->deadline;
        next->pending = false;
        next->work.handler(&next->work);
    }
    CHECK(false);
    return 1;
}

static int pixels_match(uint8_t red, uint8_t green, uint8_t blue, size_t count) {
    for (size_t index = 0; index < 4; index++) {
        struct led_rgb expected = index < count ? (struct led_rgb){red, green, blue}
                                                : (struct led_rgb){0};
        if (memcmp(&fake_pixels[index], &expected, sizeof(expected)) != 0) {
            return false;
        }
    }
    return true;
}

static void connect_slot(uint8_t slot) {
    struct bt_conn conn = {.slot = slot};
    led_feedback_connected(&conn, 0);
}

static void disconnect_slot(uint8_t slot) {
    struct bt_conn conn = {.slot = slot};
    led_feedback_disconnected(&conn, 0);
}

static void report_battery(uint8_t slot, uint8_t level) {
    zmk_event_t event = {.source = slot, .state_of_charge = level};
    battery_listener(&event);
}

static int test_priority_and_restore(void) {
    CHECK(led_feedback_init() == 0);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(64, 0, 0, 2));
    connect_slot(0);
    report_battery(0, 10);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(64, 0, 0, 2));
    CHECK(warning_state.battery_status == LF_WARNING_BATTERY_PENDING);
    fake_layer = 1;
    layer_listener(NULL);
    zmk_led_feedback_show_battery(1);
    zmk_led_feedback_change_brightness(1);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(80, 0, 0, 2));
    CHECK(!battery_work.pending);
    zmk_led_feedback_set_layer_always_on(1);
    connect_slot(1);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(80, 0, 0, 4));
    CHECK(run_until(9000) == 0);
    CHECK(pixels_match(0, 80, 0, 2));
    CHECK(!warning_rendered);
    return 0;
}

static int test_display_controls(void) {
    CHECK(led_feedback_init() == 0);
    connect_slot(0);
    connect_slot(1);
    report_battery(0, 10);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(64, 0, 0, 4));
    zmk_led_feedback_set_enabled(0);
    CHECK(pixels_match(0, 0, 0, 0));
    CHECK(run_until(0) == 0);
    zmk_led_feedback_change_brightness(1);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(0, 80, 0, 4));
    CHECK(warning_state.battery_status == LF_WARNING_BATTERY_DONE);
    CHECK(run_until(1000) == 0);
    CHECK(pixels_match(0, 0, 0, 0));
    return 0;
}

static int test_controls_during_battery_warning(void) {
    CHECK(led_feedback_init() == 0);
    connect_slot(0);
    connect_slot(1);
    report_battery(0, 10);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(64, 0, 0, 4));

    fake_layer = 3;
    layer_listener(NULL);
    zmk_led_feedback_show_battery(1);
    zmk_led_feedback_set_layer_always_on(1);
    zmk_led_feedback_change_brightness(1);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(80, 0, 0, 4));
    CHECK(warning_state.battery_status == LF_WARNING_BATTERY_ACTIVE);
    CHECK(!battery_work.pending);
    CHECK(!layer_show_work.pending);

    CHECK(run_until(9000) == 0);
    CHECK(warning_state.battery_status == LF_WARNING_BATTERY_DONE);
        CHECK(pixels_match(0, 0, 80, 4));
    return 0;
}

static int test_brightness_stops_at_bounds(void) {
    CHECK(led_feedback_init() == 0);
    connect_slot(0);
    connect_slot(1);
    CHECK(run_until(0) == 0);
    for (int step = 0; step < 10; step++) {
        zmk_led_feedback_change_brightness(1);
    }
    CHECK(brightness == CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(0, 128, 0, 4));

    for (int step = 0; step < 10; step++) {
        zmk_led_feedback_change_brightness(-1);
    }
    CHECK(brightness == CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(0, 10, 0, 4));
    return 0;
}

static int test_early_connection(void) {
    connect_slot(0);
    connect_slot(1);
    CHECK(led_feedback_init() == 0);
    CHECK(run_until(0) == 0);
    CHECK(!warning_state.connection_active);
    CHECK(pixels_match(0, 0, 0, 0));
    return 0;
}

static int test_battery_only_feedback(void) {
    CHECK(led_feedback_init() == 0);
    connect_slot(0);
    connect_slot(1);
    CHECK(run_until(20000) == 0);
    disconnect_slot(1);
    report_battery(1, 0);
    zmk_led_feedback_show_battery(2);
    CHECK(run_until(20500) == 0);
    CHECK(pixels_match(0, 0, 64, 1));
    CHECK(run_until(21500) == 0);
    CHECK(pixels_match(0, 0, 0, 0));
    return 0;
}

static int test_interrupted_battery_display(void) {
    CHECK(led_feedback_init() == 0);
    connect_slot(0);
    connect_slot(1);
    report_battery(0, 80);
    CHECK(run_until(0) == 0);
    zmk_led_feedback_show_battery(1);
    CHECK(run_until(500) == 0);
    CHECK(pixels_match(0, 64, 0, 4));
    disconnect_slot(1);
    CHECK(run_until(500) == 0);
    CHECK(pixels_match(64, 0, 0, 2));
    CHECK(!battery_work.pending);
    connect_slot(1);
    CHECK(run_until(500) == 0);
    CHECK(pixels_match(0, 0, 0, 0));
    CHECK(run_until(2500) == 0);
    CHECK(pixels_match(0, 0, 0, 0));
    return 0;
}

static int test_battery_only_required_filter(void) {
    CHECK(led_feedback_init() == 0);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(0, 0, 0, 0));
    connect_slot(1);
    report_battery(1, 10);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(0, 0, 0, 0));
    connect_slot(0);
    report_battery(0, 10);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(64, 0, 0, 4));
    CHECK(run_until(9000) == 0);
    CHECK(pixels_match(0, 0, 0, 0));
    return 0;
}

static int test_battery_restart(void) {
    CHECK(led_feedback_init() == 0);
    connect_slot(0);
    connect_slot(1);
    report_battery(0, 10);
    CHECK(run_until(500) == 0);
    CHECK(pixels_match(64, 64, 0, 4));
    disconnect_slot(1);
    CHECK(run_until(500) == 0);
    CHECK(pixels_match(64, 0, 0, 2));
    connect_slot(1);
    CHECK(run_until(500) == 0);
    CHECK(pixels_match(64, 0, 0, 4));
    CHECK(run_until(9499) == 0);
    CHECK(warning_state.battery_status == LF_WARNING_BATTERY_ACTIVE);
    CHECK(run_until(9500) == 0);
    CHECK(pixels_match(0, 0, 0, 0));
    CHECK(warning_state.battery_status == LF_WARNING_BATTERY_DONE);
    return 0;
}

static int test_delayed_deadline(void) {
    CHECK(led_feedback_init() == 0);
    connect_slot(0);
    connect_slot(1);
    CHECK(run_until(19900) == 0);
    disconnect_slot(0);
    CHECK(run_until(19900) == 0);
    CHECK(pixels_match(64, 0, 0, 1));
    fake_now_ms = 20001;
    disconnect_slot(1);
    CHECK(warning_state.boot_closed);
    CHECK(warning_state.eligible_missing_slots == 1);
    CHECK(run_until(20001) == 0);
    CHECK(pixels_match(64, 0, 0, 1));
    connect_slot(0);
    CHECK(run_until(20001) == 0);
    CHECK(pixels_match(0, 0, 0, 0));
    CHECK(!warning_state.connection_active);
    return 0;
}

static int test_display_off_before_init(void) {
    zmk_led_feedback_set_enabled(0);
    CHECK(led_feedback_init() == 0);
    CHECK(run_until(0) == 0);
    CHECK(!warning_state.display_enabled);
    CHECK(pixels_match(0, 0, 0, 0));
    connect_slot(0);
    connect_slot(1);
    report_battery(0, 10);
    CHECK(run_until(0) == 0);
    CHECK(warning_state.battery_status == LF_WARNING_BATTERY_PENDING);
    CHECK(pixels_match(0, 0, 0, 0));
    zmk_led_feedback_set_enabled(1);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(64, 0, 0, 4));
    return 0;
}

static int test_layer_hooks(void) {
    CHECK(led_feedback_init() == 0);
    connect_slot(0);
    connect_slot(1);
    CHECK(run_until(0) == 0);
    fake_layer = 3;
    layer_listener(NULL);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(0, 0, 64, 4));
    CHECK(run_until(1000) == 0);
    CHECK(pixels_match(0, 0, 0, 0));
    return 0;
}

static int test_mapped_required_filter(void) {
    CHECK(led_feedback_init() == 0);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(64, 0, 0, 1));
    connect_slot(1);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(0, 0, 0, 0));
    report_battery(0, 10);
    CHECK(run_until(0) == 0);
    CHECK(pixels_match(0, 0, 0, 0));
    zmk_led_feedback_show_battery(2);
    CHECK(run_until(500) == 0);
    CHECK(pixels_match(64, 0, 0, 1));
    report_battery(1, 10);
    CHECK(run_until(500) == 0);
    CHECK(pixels_match(64, 0, 0, 4));
    CHECK(!battery_work.pending);
    return 0;
}

struct runtime_case {
    const char *name;
    int (*run)(void);
};

int main(int argc, char **argv) {
    static const struct runtime_case cases[] = {
        {"priority_and_restore", test_priority_and_restore},
        {"display_controls", test_display_controls},
        {"controls_during_battery_warning", test_controls_during_battery_warning},
        {"brightness_stops_at_bounds", test_brightness_stops_at_bounds},
        {"early_connection", test_early_connection},
        {"battery_only_feedback", test_battery_only_feedback},
        {"interrupted_battery_display", test_interrupted_battery_display},
        {"battery_only_required_filter", test_battery_only_required_filter},
        {"battery_restart", test_battery_restart},
        {"delayed_deadline", test_delayed_deadline},
        {"display_off_before_init", test_display_off_before_init},
        {"layer_hooks", test_layer_hooks},
        {"mapped_required_filter", test_mapped_required_filter},
    };
    if (argc != 2) {
        fprintf(stderr, "Expected one runtime test case name\n");
        return 2;
    }
    for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); index++) {
        if (strcmp(argv[1], cases[index].name) == 0) {
            return cases[index].run();
        }
    }
    fprintf(stderr, "Unknown runtime test case: %s\n", argv[1]);
    return 2;
}