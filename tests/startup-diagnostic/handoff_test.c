#include <assert.h>
#include <string.h>

#include "mock.h"
#include "../../src/led_feedback.c"

const struct device mock_device = {0};
static int64_t uptime_ms;
static int strip_updates;
static struct led_rgb last_pixels[4];
static int current_layer;

int64_t k_uptime_get(void) { return uptime_ms; }
bool device_is_ready(const struct device *device) { return true; }
int zmk_keymap_highest_layer_active(void) { return current_layer; }
int led_strip_update_rgb(const struct device *device, struct led_rgb *values, size_t count) {
    assert(count == 4);
    memcpy(last_pixels, values, sizeof(last_pixels));
    strip_updates++;
    return 0;
}

static void reset_test(void) {
    uptime_ms = 19999;
    strip_updates = 0;
    current_layer = 0;
    leds_enabled = false;
    diagnostic_display_explicit = 0;
    diagnostic_finished = 0;
    diagnostic_pattern = (struct startup_pattern){0};
    slot_connected[0] = 1;
    slot_connected[1] = 1;
    battery_level[0] = 10;
    battery_level[1] = 75;
    layer_active = false;
    layer_always_on = false;
    layer_feedback_requested = false;
    layer_change_requested = false;
    brightness_preview = false;
    blink_on = false;
    battery_active = false;
    battery_warning = false;
#if CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL > 0
    battery_warned = false;
    battery_warning_reminder = false;
    warning_repeat_work = (struct k_work_delayable){0};
#endif
    layer_show_work = (struct k_work_delayable){0};
    layer_clear_work = (struct k_work_delayable){0};
    battery_work = (struct k_work_delayable){0};
    warning_work = (struct k_work_delayable){0};
    blink_work = (struct k_work_delayable){0};
    diagnostic_work = (struct k_work_delayable){0};
}

static void *run_diagnostic(void *arg) {
    diagnostic_handler(NULL);
    return NULL;
}

static void test_complete_startup(void) {
    reset_test();
    uptime_ms = 0;
    slot_connected[0] = 1;
    slot_connected[1] = 0;
    battery_level[0] = 0;
    battery_level[1] = 0;
    diagnostic_handler(NULL);
    assert(diagnostic_pattern.warning == STARTUP_WARNING_CONNECTION);
    assert(last_pixels[0].r > 0 && last_pixels[0].g == 0);

    uptime_ms = 100;
    slot_connected[1] = 1;
    diagnostic_handler(NULL);
    assert(diagnostic_pattern.warning == STARTUP_WARNING_MISSING_BATTERY);
    assert(last_pixels[0].r > 0 && last_pixels[0].g > 0);

    uptime_ms = 200;
    struct zmk_peripheral_battery_state_changed event = {.source = 0, .state_of_charge = 10};
    battery_listener(&event);
    diagnostic_handler(NULL);
    assert(diagnostic_pattern.warning == STARTUP_WARNING_LOW_BATTERY);
    assert(last_pixels[0].r > 0 && last_pixels[0].g == 0);

    uptime_ms = 300;
    zmk_led_feedback_show_battery(1);
    diagnostic_handler(NULL);
    assert(battery_active && diagnostic_pattern.warning == STARTUP_WARNING_NONE);
    battery_handler(NULL);
    assert(last_pixels[0].r > 0);
    uptime_ms = 1800;
    battery_handler(NULL);
    diagnostic_handler(NULL);
    assert(diagnostic_pattern.warning == STARTUP_WARNING_LOW_BATTERY);
    assert(last_pixels[0].r > 0 && last_pixels[0].g == 0);

    uptime_ms = 20000;
    diagnostic_handler(NULL);
    assert(diagnostic_finished && !display_enabled());
    assert(battery_level[0] == 10 && slot_connected[1]);
#if CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL > 0
    assert(battery_warned && warning_repeat_work.scheduled);
#endif
    int updates = strip_updates;
    diagnostic_handler(NULL);
    assert(strip_updates == updates && !diagnostic_work.scheduled);

    reset_test();
    uptime_ms = 19000;
    zmk_led_feedback_set_enabled(1);
    slot_connected[0] = 0;
    battery_level[0] = 75;
    diagnostic_handler(NULL);
    assert(diagnostic_pattern.warning == STARTUP_WARNING_CONNECTION);
    uptime_ms = 20000;
    diagnostic_handler(NULL);
    assert(blink_work.scheduled);
    blink_handler(NULL);
    assert(last_pixels[0].r > 0 && last_pixels[0].g == 0);
    set_connected(NULL, true);
    blink_handler(NULL);
    assert(last_pixels[0].r == 0 && last_pixels[0].g == 0);
}

int main(void) {
    test_complete_startup();
    reset_test();
    assert(display_enabled());
    battery_active = true;
    battery_work.scheduled = true;
    layer_active = true;
    layer_show_work.scheduled = true;
    layer_clear_work.scheduled = true;
    layer_feedback_requested = true;
    brightness_preview = true;
    diagnostic_work.scheduled = true;
    diagnostic_pattern.warning = STARTUP_WARNING_LOW_BATTERY;
    uptime_ms = 20000;
    diagnostic_handler(NULL);

    assert(diagnostic_finished);
    assert(!diagnostic_work.scheduled);
    assert(!layer_show_work.scheduled && !layer_clear_work.scheduled);
    assert(!layer_active && !layer_feedback_requested && !brightness_preview);
    assert(!battery_work.scheduled);
    assert(!display_enabled());
    assert(slot_connected[0] && slot_connected[1]);
    assert(battery_level[0] == 10 && battery_level[1] == 75);
    assert(blink_work.scheduled);
#if CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL > 0
    assert(battery_warned && warning_repeat_work.scheduled);
    assert(warning_work.scheduled == (CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES[0] != '\0'));
    if (CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES[0] == '\0') {
        zmk_led_feedback_set_enabled(1);
        assert(!warning_work.scheduled && warning_repeat_work.scheduled);
    }
#else
    assert(!warning_work.scheduled && !battery_active);
#endif
    int updates = strip_updates;
    diagnostic_refresh();
    diagnostic_handler(NULL);
    assert(!diagnostic_work.scheduled && strip_updates == updates);

    reset_test();
    zmk_led_feedback_set_enabled(1);
    zmk_led_feedback_set_layer_always_on(1);
    brightness = 42;
    current_layer = 2;
    uptime_ms = 20000;
    diagnostic_handler(NULL);
    assert(display_enabled() && layer_always_on && current_layer == 2 && brightness == 42);
#if CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL > 0
    assert(warning_work.scheduled && warning_repeat_work.scheduled);
#else
    assert(!warning_work.scheduled);
    assert(last_pixels[0].g > 0);
#endif

    reset_test();
    pthread_t pending;
    assert(k_mutex_lock(&diagnostic_mutex, K_FOREVER) == 0);
    assert(pthread_create(&pending, NULL, run_diagnostic, NULL) == 0);
    uptime_ms = 20000;
    assert(k_mutex_unlock(&diagnostic_mutex) == 0);
    assert(pthread_join(pending, NULL) == 0);
    assert(diagnostic_finished && !diagnostic_work.scheduled);
    updates = strip_updates;
    diagnostic_handler(NULL);
    assert(strip_updates == updates && !diagnostic_work.scheduled);
    return 0;
}
