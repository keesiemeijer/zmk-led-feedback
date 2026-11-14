/*
 * Copyright (c) 2026 keesiemeijer
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>

#include "warning_state.h"
#include <zmk_led_feedback.h>

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_CONNECTION) || IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
#define LF_TRACK_WARNINGS 1
#else
#define LF_TRACK_WARNINGS 0
#endif

// LED strip configuration and initialization.
// zmk_led_feedback node identifier for the /chosen node.
#define STRIP_NODE DT_CHOSEN(zmk_led_feedback)

// Length of the LED strip.
#define STRIP_LEN DT_PROP(STRIP_NODE, chain_length)

// Brightness (0-255), never above the user max brightness. Changed at runtime with zmk_led_feedback_change_brightness().
BUILD_ASSERT(CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS <= CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS,
             "CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS is higher than MAX_BRIGHTNESS");
BUILD_ASSERT(CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS >= CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS &&
                 CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS <= CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS,
             "CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS must be between MIN_BRIGHTNESS and MAX_BRIGHTNESS");
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
BUILD_ASSERT(CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT <= STRIP_LEN,
             "CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT exceeds strip length");
#endif

// Current brightness of the LED strip. Can be changed at runtime.
static uint8_t brightness = CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS;

// Macro to scale a color channel by the current brightness.
#define SCALE(channel) ((channel) * brightness / 255)

// LED strip device and pixel buffer.
static const struct device *strip = DEVICE_DT_GET(STRIP_NODE);
static K_MUTEX_DEFINE(strip_lock);

// Pixel buffer for the LED strip. Each element represents an RGB LED.
static struct led_rgb pixels[STRIP_LEN];

// Feedback sources for the LED strip.
// Determines what kind of information is currently being displayed.
enum feedback_source {
    FEEDBACK_NONE,
    FEEDBACK_LAYER,
    FEEDBACK_LAYER_CHANGE,
    FEEDBACK_BATTERY,
    FEEDBACK_BATTERY_WARNING,
    FEEDBACK_CONNECTION_WARNING,
};

static atomic_t leds_enabled = ATOMIC_INIT(1);

#if LF_TRACK_WARNINGS
static struct lf_warning_state warning_state;
static struct k_spinlock warning_lock;
static bool warning_ready;
static uint32_t early_connected_slots;
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
static uint32_t early_battery_slots;
static uint8_t early_battery_levels[LF_WARNING_MAX_PERIPHERALS];
#endif
static bool warning_rendered;

static void warning_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(warning_work, warning_work_handler);
#endif

static bool feedback_can_show(enum feedback_source source) {
    if (!atomic_get(&leds_enabled)) {
        return false;
    }
#if LF_TRACK_WARNINGS
    k_spinlock_key_t key = k_spin_lock(&warning_lock);
    enum lf_warning_output_type active = warning_ready ? lf_warning_get_output(&warning_state).type
                                                       : LF_WARNING_OUTPUT_NONE;
    k_spin_unlock(&warning_lock, key);
    if (active == LF_WARNING_OUTPUT_CONNECTION) {
        return source == FEEDBACK_CONNECTION_WARNING;
    }
    if (active == LF_WARNING_OUTPUT_BATTERY) {
        return source == FEEDBACK_BATTERY_WARNING;
    }
#endif
    return source != FEEDBACK_CONNECTION_WARNING && source != FEEDBACK_BATTERY_WARNING;
}

// Updates the LED strip with the specified number of lit LEDs and color.
// The LEDs fill up from the last to the first if reversed is enabled.
static void fill_pixels_locked(int lit, struct led_rgb color, bool visible) {
    for (int i = 0; i < STRIP_LEN; i++) {
        // Reversed: LEDs fill up from the last LED to the first.
        int pos = IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_REVERSE) ? STRIP_LEN - 1 - i : i;
        pixels[pos] = visible && i < lit ? color : (struct led_rgb){0};
    }
}

static void update_strip_locked(int lit, struct led_rgb color, enum feedback_source source) {
    bool visible = feedback_can_show(source);
    if (!visible && atomic_get(&leds_enabled)) {
        return;
    }

    fill_pixels_locked(lit, color, visible);
    led_strip_update_rgb(strip, pixels, STRIP_LEN);
}

static void update_strip(int lit, struct led_rgb color, enum feedback_source source) {
    k_mutex_lock(&strip_lock, K_FOREVER);
    update_strip_locked(lit, color, source);
    k_mutex_unlock(&strip_lock);
}

static struct led_rgb scale_rgb(uint32_t rgb) {
    return (struct led_rgb){
        .r = SCALE((rgb >> 16) & 0xFF),
        .g = SCALE((rgb >> 8) & 0xFF),
        .b = SCALE(rgb & 0xFF),
    };
}

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
// True while the battery levels are shown. Layer changes cancel it.
static bool battery_active;
static void battery_cancel(void);
#endif

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)

static bool layer_active;
static bool brightness_preview;
static bool layer_feedback_requested;
static bool layer_change_requested;
static bool layer_always_on = IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON);
#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
static bool layer_timeout_expired;
static bool layer_timeout_started;
static bool layer_continuous_visible;
#if !LF_TRACK_WARNINGS
static uint64_t layer_timeout_boot_deadline_ms;
#endif
#endif

// True when layer is an entry of a comma separated list of layer numbers.
// Invalid entries are ignored.
static bool layer_in_list(const char *list, int layer) {
    while (*list) {
        char *end;
        long entry = strtol(list, &end, 10);

        if (end != list && entry == layer) {
            return true;
        }

        list = strchr(end, ',');
        if (!list) {
            break;
        }
        list++;
    }

    return false;
}

// Continuous layer indication is enabled globally or for individual layers.
static bool layer_continuous_enabled(void) {
    return layer_always_on ||
           layer_in_list(CONFIG_ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON_LAYERS,
                         zmk_keymap_highest_layer_active());
}

static bool layer_stays_on(void) {
#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
    return !layer_timeout_expired && layer_continuous_enabled();
#else
    return layer_continuous_enabled();
#endif
}

// Layer show and clear work handlers. These are used to update the LED strip for layer changes.
static void layer_show_handler(struct k_work *work);
static void layer_clear_handler(struct k_work *work);
#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
static void layer_timeout_handler(struct k_work *work);
static void layer_timeout_gate_handler(struct k_work *work);
static void layer_timeout_maybe_start(void);
static void layer_timeout_note_activity(void);
#endif

// Delayable work items for layer show and clear. These allow the LED updates to be scheduled with a delay.
static K_WORK_DELAYABLE_DEFINE(layer_show_work, layer_show_handler);
static K_WORK_DELAYABLE_DEFINE(layer_clear_work, layer_clear_handler);
#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
static K_WORK_DELAYABLE_DEFINE(layer_timeout_work, layer_timeout_handler);
static K_WORK_DELAYABLE_DEFINE(layer_timeout_gate_work, layer_timeout_gate_handler);
#endif

// Parses the "RRGGBB" entry of a comma separated list.
static uint32_t palette_rgb(const char *list, int idx) {
    int count = 1;
    for (const char *p = list; *p; p++) {
        if (*p == ',') {
            count++;
        }
    }

    const char *entry = list;
    for (int i = idx % count; i > 0; i--) {
        entry = strchr(entry, ',') + 1;
    }

    char hex[7] = {0};
    strncpy(hex, entry, 6);
    char *comma = strchr(hex, ',');
    if (comma) {
        *comma = '\0';
    }

    return strtoul(hex, NULL, 16);
}

// Returns the length of the LED strip.
int zmk_led_feedback_strip_length(void) {
    return STRIP_LEN;
}

// Gets the color from the main palette for the specified index.
uint32_t zmk_led_feedback_palette_color(int index) {
    return palette_rgb(CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS, index);
}

// Gets the color for the specified layer from the palette. Uses per-layer colors if available
static uint32_t layer_palette_color(int layer) {
    if (CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER[0] != '\0') {
        return palette_rgb(CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER, layer);
    }

    return palette_rgb(CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS, layer / STRIP_LEN);
}

// Can be overridden by the user to change the number of LEDs lit for a layer.
// Only used for the layer indication.
__weak int zmk_led_feedback_layer_led_count(int layer, int count) {
    return count;
}

// Can be overridden by the user to change the color (0xRRGGBB) of a layer
// indication. It gets the number of LEDs after the count hook and validation.
// Only used for the layer indication.
__weak uint32_t zmk_led_feedback_layer_color(int layer, int led_count, uint32_t color) {
    return color;
}

// Number of LEDs lit for a layer. With per layer colors the LED count option can light all LEDs
// (-1) or a fixed number of LEDs (N). Otherwise layer N lights (N % STRIP_LEN) + 1 LEDs.
static int layer_led_count(int layer) {
    int setting = CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT;

    if (CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER[0] != '\0' && setting != 0) {
        return setting < 0 ? STRIP_LEN : MIN(setting, STRIP_LEN);
    }

    return (layer % STRIP_LEN) + 1;
}

// Draws the layer indication on the LED strip.
// If preview is true, it shows a preview of the first layer.
// The source parameter indicates the feedback source for the update.
static void draw_layer(bool preview, enum feedback_source source) {
    int layer = zmk_keymap_highest_layer_active();
    uint32_t rgb;
    int lit;

    // Preview for the brightness keys: all LEDs in the first layer color.
    if (preview) {
        rgb = layer_palette_color(0);
        lit = STRIP_LEN;
    } else {
        rgb = layer_palette_color(layer);
        lit = layer_led_count(layer);
    }

    if (!preview) {
        lit = CLAMP(zmk_led_feedback_layer_led_count(layer, lit), 0, STRIP_LEN);
        rgb = zmk_led_feedback_layer_color(layer, lit, rgb) & 0xFFFFFF;
    }

    struct led_rgb color = scale_rgb(rgb);
    update_strip(lit, color, source);
}

// Layer show handler. This updates the LED strip when a layer becomes active.
static void layer_show_handler(struct k_work *work) {
    enum feedback_source source = FEEDBACK_NONE;
    if (layer_feedback_requested) {
        source = FEEDBACK_LAYER;
    } else if (layer_change_requested) {
        source = FEEDBACK_LAYER_CHANGE;
    }

    layer_feedback_requested = false;
    layer_change_requested = false;
    bool preview = brightness_preview;
    brightness_preview = false;
#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
    layer_continuous_visible = false;
#endif

    // Check if feedback can be shown for the current source. If not, deactivate the layer and return.
    if (!feedback_can_show(source)) {
        layer_active = false;
        return;
    }

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
    battery_cancel();
#endif
    draw_layer(preview, source);
    layer_active = true;
    k_work_reschedule(&layer_clear_work, K_MSEC(CONFIG_ZMK_LED_FEEDBACK_LAYER_DURATION_MS));
}

// Layer clear handler. This clears the LED strip when a layer is no longer active.
static void layer_clear_handler(struct k_work *work) {
    layer_active = false;
#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
    layer_continuous_visible = false;
#endif

    if (!feedback_can_show(FEEDBACK_NONE)) {
        return;
    }

    if (layer_stays_on()) {
        draw_layer(false, FEEDBACK_NONE);
#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
        layer_continuous_visible = true;
#endif
    } else {
        update_strip(0, (struct led_rgb){0}, FEEDBACK_NONE);
    }
}

#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
static bool layer_timeout_conditions_met(void) {
    if (!atomic_get(&leds_enabled)) {
        return false;
    }

#if LF_TRACK_WARNINGS
    k_spinlock_key_t key = k_spin_lock(&warning_lock);
    bool ready = warning_ready && warning_state.boot_closed &&
                 lf_warning_get_output(&warning_state).type == LF_WARNING_OUTPUT_NONE;
    k_spin_unlock(&warning_lock, key);
    return ready;
#else
    return k_uptime_get() >= layer_timeout_boot_deadline_ms;
#endif
}

static void layer_timeout_maybe_start(void) {
    if (layer_timeout_started || layer_timeout_expired || !layer_timeout_conditions_met()) {
        return;
    }

    layer_timeout_started = true;
    k_work_reschedule(&layer_timeout_work,
                      K_MSEC(CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES *
                             60 * 1000));
}

static void layer_timeout_handler(struct k_work *work) {
    layer_timeout_started = false;
    if (!layer_timeout_conditions_met()) {
        layer_timeout_maybe_start();
        return;
    }

    layer_timeout_expired = true;
    if (layer_continuous_visible) {
        layer_clear_handler(NULL);
    }
}

static void layer_timeout_gate_handler(struct k_work *work) {
#if !LF_TRACK_WARNINGS
    uint64_t now_ms = k_uptime_get();
    if (now_ms < layer_timeout_boot_deadline_ms) {
        k_work_reschedule(&layer_timeout_gate_work,
                          K_MSEC(layer_timeout_boot_deadline_ms - now_ms));
        return;
    }
#endif
    layer_timeout_maybe_start();
}

static void layer_timeout_note_activity(void) {
    bool restore_continuous = layer_timeout_expired && layer_continuous_enabled();
    layer_timeout_expired = false;

    if (layer_timeout_started) {
        k_work_reschedule(&layer_timeout_work,
                          K_MSEC(CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES *
                                 60 * 1000));
    } else {
        layer_timeout_maybe_start();
    }

    bool other_feedback_active = layer_active;
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
    other_feedback_active = other_feedback_active || battery_active;
#endif
    if (restore_continuous && !other_feedback_active && device_is_ready(strip) &&
        feedback_can_show(FEEDBACK_LAYER)) {
        layer_feedback_requested = true;
        k_work_reschedule(&layer_show_work, K_NO_WAIT);
    }
}
#endif

// ZMK layer events listener. Schedules the layer_show_handler when the layer state changes.
static int layer_listener(const zmk_event_t *eh) {
    if (device_is_ready(strip) && feedback_can_show(FEEDBACK_LAYER_CHANGE)) {
        layer_change_requested = true;
        k_work_reschedule(&layer_show_work, K_NO_WAIT);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
static int position_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *event = as_zmk_position_state_changed(eh);
    if (event && event->state) {
        layer_timeout_note_activity();
    }
    return ZMK_EV_EVENT_BUBBLE;
}
#endif

// Initializes the layer feedback. Schedules the layer_show_handler if the layer should stay on.
static int layer_init(void) {
#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
#if !LF_TRACK_WARNINGS
    layer_timeout_boot_deadline_ms = k_uptime_get() + LF_WARNING_BOOT_MS;
#endif
    k_work_schedule(&layer_timeout_gate_work, K_MSEC(LF_WARNING_BOOT_MS));
    layer_timeout_maybe_start();
#endif
    if (layer_stays_on()) {
        k_work_reschedule(&layer_show_work, K_NO_WAIT);
    }
    return 0;
}

SYS_INIT(layer_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

// Register the LED feedback layer listener with the ZMK event system.
ZMK_LISTENER(led_feedback_layer, layer_listener);
// ZMK subscription for the layer listener.
ZMK_SUBSCRIPTION(led_feedback_layer, zmk_layer_state_changed);
#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
ZMK_LISTENER(led_feedback_position, position_listener);
ZMK_SUBSCRIPTION(led_feedback_position, zmk_position_state_changed);
#endif

#endif // IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_CONNECTION) || IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)

//     Peripheral slot configuration for connection and battery feedback.
#define PERIPHERALS CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS

static int slot_number(int slot) {
    const char *entry = CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER;
    if (!*entry) {
        return slot + 1;
    }

    for (int i = slot; i > 0; i--) {
        entry = strchr(entry, ',');
        entry++;
    }

    return (int)strtol(entry, NULL, 10);
}

#endif

#if LF_TRACK_WARNINGS

BUILD_ASSERT(PERIPHERALS <= LF_WARNING_MAX_PERIPHERALS,
             "Too many peripherals for LED feedback warnings");
BUILD_ASSERT(STRIP_LEN > 0 && STRIP_LEN <= UINT16_MAX,
             "LED feedback strip length is invalid");

static void warning_schedule_locked(void) {
    k_work_reschedule(&warning_work, K_NO_WAIT);
}

static void warning_connection_event(uint8_t slot, bool connected) {
    if (slot >= PERIPHERALS || slot >= LF_WARNING_MAX_PERIPHERALS) {
        return;
    }
    k_spinlock_key_t key = k_spin_lock(&warning_lock);
    uint32_t bit = UINT32_C(1) << slot;
    if (connected) {
        early_connected_slots |= bit;
    } else {
        early_connected_slots &= ~bit;
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
        early_battery_slots &= ~bit;
        early_battery_levels[slot] = 0;
#endif
    }
    if (warning_ready) {
        lf_warning_connection_event(&warning_state, k_uptime_get(), slot, connected);
        warning_schedule_locked();
    }
    k_spin_unlock(&warning_lock, key);
}

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
static void warning_battery_event(uint8_t slot, uint8_t level) {
    if (slot >= PERIPHERALS || slot >= LF_WARNING_MAX_PERIPHERALS) {
        return;
    }
    k_spinlock_key_t key = k_spin_lock(&warning_lock);
    if (warning_ready) {
        lf_warning_battery_event(&warning_state, k_uptime_get(), slot, level);
        warning_schedule_locked();
    } else {
        early_battery_levels[slot] = level;
        early_battery_slots |= UINT32_C(1) << slot;
    }
    k_spin_unlock(&warning_lock, key);
}

static uint8_t warning_battery_level(int slot) {
    if (slot < 0 || slot >= PERIPHERALS) {
        return 0;
    }
    k_spinlock_key_t key = k_spin_lock(&warning_lock);
    uint8_t level = warning_ready ? warning_state.battery_levels[slot] : 0;
    k_spin_unlock(&warning_lock, key);
    return level;
}
#endif

static void set_connected(struct bt_conn *conn, bool connected) {
    struct bt_conn_info info;

    if (bt_conn_get_info(conn, &info) < 0 || info.role != BT_CONN_ROLE_CENTRAL) {
        return;
    }

    int slot = zmk_ble_put_peripheral_addr(bt_conn_get_dst(conn));
    if (slot >= 0 && slot < PERIPHERALS) {
        warning_connection_event(slot, connected);
    }
}

// Handles a successful Bluetooth connection event.
static void led_feedback_connected(struct bt_conn *conn, uint8_t err) {
    if (!err) {
        set_connected(conn, true);
    }
}

// Handles a Bluetooth disconnection event.
static void led_feedback_disconnected(struct bt_conn *conn, uint8_t reason) {
    set_connected(conn, false);
}

BT_CONN_CB_DEFINE(led_feedback_conn_cb) = {
    .connected = led_feedback_connected,
    .disconnected = led_feedback_disconnected,
};

static int led_feedback_init(void) {
    struct lf_warning_config config = {
        .peripheral_count = PERIPHERALS,
        .required_count = CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS,
        .connection_enabled = IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_CONNECTION),
        .blink_interval_ms = CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS,
        .strip_length = STRIP_LEN,
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
        .battery_threshold = CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL,
        .battery_blinks = CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS,
#else
        .battery_blinks = 5,
#endif
    };
    for (int slot = 0; slot < PERIPHERALS; slot++) {
        config.device_numbers[slot] = slot_number(slot);
    }

    k_spinlock_key_t key = k_spin_lock(&warning_lock);
    uint64_t now_ms = k_uptime_get();
    if (!lf_warning_init(&warning_state, &config, now_ms)) {
        k_spin_unlock(&warning_lock, key);
        return -EINVAL;
    }
    if (!atomic_get(&leds_enabled)) {
        lf_warning_set_display(&warning_state, now_ms, false);
    }
    for (int slot = 0; slot < PERIPHERALS; slot++) {
        uint32_t bit = UINT32_C(1) << slot;
        if (early_connected_slots & bit) {
            lf_warning_connection_event(&warning_state, now_ms, slot, true);
        }
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
        if (early_battery_slots & bit) {
            lf_warning_battery_event(&warning_state, now_ms, slot, early_battery_levels[slot]);
        }
#endif
    }
    warning_ready = true;
    warning_schedule_locked();
    k_spin_unlock(&warning_lock, key);
    return 0;
}

SYS_INIT(led_feedback_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif // LF_TRACK_WARNINGS

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)

// The device numbers still to show, and whether the last step was a display (not a gap).
static int battery_number;
static int battery_last;
static bool battery_shown;

static void battery_handler(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(battery_work, battery_handler);

#define BATTERY_COLORS CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS

// The format is checked in CMakeLists.txt.
// Color (0xRRGGBB) of BATTERY_COLORS entry idx: 0 high, 1 medium, 2 low, 3 no reading.
static uint32_t battery_color(int idx) {
    return strtoul(BATTERY_COLORS + idx * 7, NULL, 16) & 0xFFFFFF;
}

// Returns the peripheral slot index for a given device number, or -1 if not found.
static int battery_slot(int number) {
    for (int i = 0; i < PERIPHERALS; i++) {
        if (slot_number(i) == number) {
            return i;
        }
    }

    return -1;
}

static void battery_cancel(void) {
    battery_active = false;
    k_work_cancel_delayable(&battery_work);
}

static void battery_finish(void) {
    battery_active = false;

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
    // Clear the layer indication before finishing.
    layer_clear_handler(NULL);
#else
    // Clear the strip if layer feedback is not enabled.
    update_strip(0, (struct led_rgb){0}, FEEDBACK_NONE);
#endif
}

// Shows one device at a time. The number of LEDs follows the charge and the color goes from
// green to yellow to red. A single blue LED means there is no reading.
static void battery_handler(struct k_work *work) {
    if (!feedback_can_show(FEEDBACK_BATTERY)) {
        battery_cancel();
        return;
    }
    if (battery_shown) {
        battery_shown = false;

        battery_number++;

        if (battery_number > battery_last) {
            battery_finish();
            return;
        }

        // Short gap so two devices with the same level can be told apart.
        update_strip(0, (struct led_rgb){0}, FEEDBACK_BATTERY);
        k_work_reschedule(&battery_work, K_MSEC(150));
        return;
    }

    int slot = battery_slot(battery_number);
    uint8_t level = warning_battery_level(slot);
    int lit = 1;
    uint32_t rgb = battery_color(3);

    if (level > 0) {
        lit = CLAMP(DIV_ROUND_UP(level * STRIP_LEN, 100), 1, STRIP_LEN);
        if (level > 60) {
            // If the level is above 60, use high color;
            rgb = battery_color(0);
        } else if (level >= 20) {
            // if between 20 and 60, use medium color.
            rgb = battery_color(1);
        } else {
            // if below 20, use low color.
            rgb = battery_color(2);
        }
    }

    update_strip(lit, scale_rgb(rgb), FEEDBACK_BATTERY);
    battery_shown = true;
    k_work_reschedule(&battery_work, K_MSEC(CONFIG_ZMK_LED_FEEDBACK_BATTERY_DURATION_MS));
}

// Listener for peripheral battery state changes. Updates the battery level and triggers warnings if necessary.
static int battery_listener(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);

    if (ev && ev->source < PERIPHERALS) {
        warning_battery_event(ev->source, ev->state_of_charge);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

// Registers the LED feedback battery listener with the ZMK event system.
ZMK_LISTENER(led_feedback_battery, battery_listener);
// ZMK subscription for the battery listener.
ZMK_SUBSCRIPTION(led_feedback_battery, zmk_peripheral_battery_state_changed);

#endif // IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)

// Shows all LEDs in the first layer color as a preview of the new brightness.
static void show_preview(void) {
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
    if (device_is_ready(strip) && feedback_can_show(FEEDBACK_LAYER)) {
        brightness_preview = true;
        k_work_reschedule(&layer_show_work, K_NO_WAIT);
    }
#endif
}

// Changes the LED brightness in the specified direction and shows a preview.
void zmk_led_feedback_change_brightness(int direction) {
    brightness = CLAMP((int)brightness + direction * CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS_STEP,
                       CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS,
                       CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS);

    zmk_led_feedback_set_enabled(1);
}

// Shows the current layer on the LED strip.
void zmk_led_feedback_show_layer(void) {
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
    if (device_is_ready(strip) && feedback_can_show(FEEDBACK_LAYER)) {
        layer_feedback_requested = true;
        k_work_reschedule(&layer_show_work, K_NO_WAIT);
    }
#endif
}

// Sets whether the layer indication should always be on.
void zmk_led_feedback_set_layer_always_on(int mode) {
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
    layer_always_on = mode < 0 ? !layer_always_on : mode > 0;
    if (device_is_ready(strip) && feedback_can_show(FEEDBACK_LAYER)) {
        k_work_reschedule(&layer_show_work, K_NO_WAIT);
    }
#endif
}

// Enables or disables the LED feedback.
void zmk_led_feedback_set_enabled(int mode) {
    bool enabled = mode < 0 ? !atomic_get(&leds_enabled) : mode > 0;
    atomic_set(&leds_enabled, enabled);
#if LF_TRACK_WARNINGS
    k_spinlock_key_t key = k_spin_lock(&warning_lock);
    if (warning_ready) {
        lf_warning_set_display(&warning_state, k_uptime_get(), enabled);
        warning_schedule_locked();
    }
    k_spin_unlock(&warning_lock, key);
#endif

    if (!device_is_ready(strip)) {
        return;
    }

    if (enabled) {
        show_preview();
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER) && \
    CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
        layer_timeout_maybe_start();
#endif
    } else {
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER) && \
    CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
        k_work_cancel_delayable(&layer_timeout_work);
        layer_timeout_started = false;
        layer_timeout_expired = false;
        layer_continuous_visible = false;
#endif
        update_strip(0, (struct led_rgb){0}, FEEDBACK_NONE);
    }
}

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
// Stops the layer display and any battery display, so the strip is free for a battery display.
static void battery_take_over(void) {
    battery_cancel();

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
    k_work_cancel_delayable(&layer_show_work);
    k_work_cancel_delayable(&layer_clear_work);
    layer_active = false;
#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
    layer_continuous_visible = false;
#endif
#endif

    battery_active = true;
}

// Starts showing the battery level for the specified device number.
static void battery_start(int number) {
    if (!device_is_ready(strip) || !feedback_can_show(FEEDBACK_BATTERY) || number < 0 ||
        number > PERIPHERALS) {
        return;
    }

    battery_take_over();
    battery_shown = false;
    battery_number = number > 0 ? number : 1;
    battery_last = number > 0 ? number : PERIPHERALS;

    // Dark pause before the first device, so it can't be mistaken for a layer indication.
    update_strip(0, (struct led_rgb){0}, FEEDBACK_BATTERY);
    k_work_reschedule(&battery_work, K_MSEC(CONFIG_ZMK_LED_FEEDBACK_BATTERY_DELAY_MS));
}
#endif

// Shows the battery level of the device with this number (0 = all devices, one after another).
void zmk_led_feedback_show_battery(int number) {
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
    battery_start(number);
#endif
}

// Handles warning work, updating the LED strip based on the current warning state.
#if LF_TRACK_WARNINGS
static void warning_work_handler(struct k_work *work) {
    if (!device_is_ready(strip)) {
        k_spinlock_key_t key = k_spin_lock(&warning_lock);
        if (warning_ready) {
            k_work_reschedule(&warning_work, K_MSEC(100));
        }
        k_spin_unlock(&warning_lock, key);
        return;
    }

    k_mutex_lock(&strip_lock, K_FOREVER);
    k_spinlock_key_t key = k_spin_lock(&warning_lock);
    if (!warning_ready) {
        k_spin_unlock(&warning_lock, key);
        k_mutex_unlock(&strip_lock);
        return;
    }

    uint64_t now_ms = k_uptime_get();
    lf_warning_advance(&warning_state, now_ms);
    struct lf_warning_output candidate =
        lf_warning_get_connection_candidate(&warning_state);
    struct lf_warning_output output = lf_warning_get_output(&warning_state);
    uint64_t deadline = lf_warning_next_deadline(&warning_state);
    k_spin_unlock(&warning_lock, key);

    if (candidate.type == LF_WARNING_OUTPUT_CONNECTION) {
        fill_pixels_locked(candidate.led_count, scale_rgb(candidate.color), true);
        key = k_spin_lock(&warning_lock);
        uint64_t phase_start_ms = k_uptime_get();
        bool rendered = lf_warning_render_connection_phase(&warning_state, phase_start_ms);
        if (!rendered) {
            output = lf_warning_get_output(&warning_state);
            deadline = lf_warning_next_deadline(&warning_state);
        }
        k_spin_unlock(&warning_lock, key);

        if (rendered) {
            if (!atomic_get(&leds_enabled)) {
                fill_pixels_locked(0, (struct led_rgb){0}, false);
            }
            led_strip_update_rgb(strip, pixels, STRIP_LEN);
            warning_rendered = true;
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
            k_work_cancel_delayable(&layer_show_work);
            k_work_cancel_delayable(&layer_clear_work);
            layer_active = false;
            layer_feedback_requested = false;
            layer_change_requested = false;
            brightness_preview = false;
#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
            layer_continuous_visible = false;
#endif
#endif
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
            battery_cancel();
#endif

            key = k_spin_lock(&warning_lock);
            now_ms = k_uptime_get();
            deadline = lf_warning_next_deadline(&warning_state);
            if (deadline != LF_WARNING_NO_DEADLINE) {
                k_work_schedule(&warning_work,
                                K_MSEC(deadline > now_ms ? deadline - now_ms : 0));
            }
            k_spin_unlock(&warning_lock, key);
            k_mutex_unlock(&strip_lock);
            return;
        }
    }

    key = k_spin_lock(&warning_lock);
    now_ms = k_uptime_get();
    deadline = lf_warning_next_deadline(&warning_state);
    if (deadline != LF_WARNING_NO_DEADLINE) {
        k_work_reschedule(&warning_work, K_MSEC(deadline > now_ms ? deadline - now_ms : 0));
    }
    k_spin_unlock(&warning_lock, key);

    if (output.type == LF_WARNING_OUTPUT_NONE) {
        if (warning_rendered) {
            warning_rendered = false;
            k_mutex_unlock(&strip_lock);
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
            layer_clear_handler(NULL);
#if CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
            layer_timeout_maybe_start();
#endif
#else
            update_strip(0, (struct led_rgb){0}, FEEDBACK_NONE);
#endif
            return;
        }
        k_mutex_unlock(&strip_lock);
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER) && \
    CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES > 0
        layer_timeout_maybe_start();
#endif
        return;
    }

    warning_rendered = true;
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
    k_work_cancel_delayable(&layer_show_work);
    k_work_cancel_delayable(&layer_clear_work);
    layer_active = false;
    layer_feedback_requested = false;
    layer_change_requested = false;
    brightness_preview = false;
#endif
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
    battery_cancel();
#endif
    enum feedback_source source = output.type == LF_WARNING_OUTPUT_CONNECTION
                                      ? FEEDBACK_CONNECTION_WARNING
                                      : FEEDBACK_BATTERY_WARNING;
    update_strip_locked(output.led_count, scale_rgb(output.color), source);
    k_mutex_unlock(&strip_lock);
}
#endif
