/*
 * Copyright (c) 2026 keesiemeijer
 *
 * SPDX-License-Identifier: MIT
 */

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
#include <zmk/keymap.h>

#include <zmk_led_feedback.h>

// Checks the battery level (if enabled) and triggers a warning if it falls below the configured threshold.
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY) && CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL > 0
static void battery_warn_check(void);
#endif

// LED strip configuration and initialization.
// zmk_led_feedback node identifier for the /chosen node.
#define STRIP_NODE DT_CHOSEN(zmk_led_feedback)

// Length of the LED strip.
#define STRIP_LEN DT_PROP(STRIP_NODE, chain_length)

// Brightness (0-255), never above the user max brightness. Changed at runtime with zmk_led_feedback_change_brightness().
BUILD_ASSERT(CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS <= CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS,
             "CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS is higher than MAX_BRIGHTNESS");

// Current brightness of the LED strip. Can be changed at runtime.
static uint8_t brightness = CLAMP(CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS,
                                  CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS,
                                  CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS);

// Macro to scale a color channel by the current brightness.
#define SCALE(channel) ((channel) * brightness / 255)

// LED strip device and pixel buffer.
static const struct device *strip = DEVICE_DT_GET(STRIP_NODE);

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

// Indicates whether the LEDs are currently enabled.
static bool leds_enabled = !IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_DISPLAY_OFF);

// Determines whether the specified feedback source can override the LED off state.
static bool feedback_can_override_off(enum feedback_source source) {
    const char *name;

    switch (source) {
    case FEEDBACK_LAYER:
        name = "layer_feedback";
        break;
    case FEEDBACK_LAYER_CHANGE:
        name = "layer_change";
        break;
    case FEEDBACK_BATTERY:
        name = "battery_feedback";
        break;
    case FEEDBACK_BATTERY_WARNING:
        name = "battery_warning";
        break;
    case FEEDBACK_CONNECTION_WARNING:
        name = "connection_warning";
        break;
    default:
        return false;
    }

    // Check if the feedback source is listed in the CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES.
    const char *entry = CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES;
    while (*entry) {
        const char *end = strchr(entry, ',');
        size_t length = end ? (size_t)(end - entry) : strlen(entry);

        if (length == strlen(name) && memcmp(entry, name, length) == 0) {
            return true;
        }

        if (!end) {
            break;
        }
        entry = end + 1;
    }

    return false;
}

// Determines whether the specified feedback source can be shown on the LEDs.
static bool feedback_can_show(enum feedback_source source) {
    return leds_enabled || feedback_can_override_off(source);
}

// Updates the LED strip with the specified number of lit LEDs and color.
// The LEDs fill up from the last to the first if reversed is enabled.
static void update_strip(int lit, struct led_rgb color, enum feedback_source source) {
    bool visible = feedback_can_show(source);

    for (int i = 0; i < STRIP_LEN; i++) {
        // Reversed: LEDs fill up from the last LED to the first.
        int pos = IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_REVERSE) ? STRIP_LEN - 1 - i : i;
        pixels[pos] = visible && i < lit ? color : (struct led_rgb){0};
    }

    led_strip_update_rgb(strip, pixels, STRIP_LEN);
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

// The layer indication stays on for all layers, or for the layers in ALWAYS_ON_LAYERS.
static bool layer_stays_on(void) {
    return layer_always_on || layer_in_list(CONFIG_ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON_LAYERS, zmk_keymap_highest_layer_active());
}

// Layer show and clear work handlers. These are used to update the LED strip for layer changes.
static void layer_show_handler(struct k_work *work);
static void layer_clear_handler(struct k_work *work);

// Delayable work items for layer show and clear. These allow the LED updates to be scheduled with a delay.
static K_WORK_DELAYABLE_DEFINE(layer_show_work, layer_show_handler);
static K_WORK_DELAYABLE_DEFINE(layer_clear_work, layer_clear_handler);

// Connection restart handler for when the connection status changes.
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_CONNECTION)
static void connection_restart(void);
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

    // Check if feedback can be shown for the current source. If not, deactivate the layer and return.
    if (!feedback_can_show(source)) {
        layer_active = false;
        return;
    }

    // Cancel any ongoing battery indication or warning before updating the layer display.
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

    if (layer_stays_on()) {
        draw_layer(false, FEEDBACK_NONE);
    } else {
        update_strip(0, (struct led_rgb){0}, FEEDBACK_NONE);
    }

    // Resume connection warnings and check for pending low-battery warnings.
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_CONNECTION)
    connection_restart();
#endif
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY) && CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL > 0
    battery_warn_check();
#endif
}

// ZMK layer events listener. Schedules the layer_show_handler when the layer state changes.
static int layer_listener(const zmk_event_t *eh) {
    if (device_is_ready(strip)) {
        layer_change_requested = true;
        k_work_reschedule(&layer_show_work, K_NO_WAIT);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

// Initializes the layer feedback. Schedules the layer_show_handler if the layer should stay on.
static int layer_init(void) {
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

#endif // IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_CONNECTION) || IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)

//     Peripheral slot configuration for connection and battery feedback.
#define PERIPHERALS CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS

// Device number of a peripheral slot (the LEDs blinked while it is missing): the matching entry of the SLOT_ORDER list, or
// slot + 1 when the entry is missing or invalid.
static int slot_number(int slot) {
    const char *entry = CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER;

    for (int i = slot; i > 0 && entry; i--) {
        entry = strchr(entry, ',');
        entry = entry ? entry + 1 : NULL;
    }

    long number = entry ? strtol(entry, NULL, 10) : 0;

    return number >= 1 && number <= PERIPHERALS ? (int)number : slot + 1;
}

#endif

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_CONNECTION)

// Number of required peripherals to consider for blinking. If negative, all peripherals are required.
#define REQUIRED (CONFIG_ZMK_LED_FEEDBACK_CONNECTION_REQUIRED_PERIPHERALS < 0 ? PERIPHERALS : MIN(CONFIG_ZMK_LED_FEEDBACK_CONNECTION_REQUIRED_PERIPHERALS, PERIPHERALS))

// One flag per peripheral, in the order the peripherals first paired with the central.
static atomic_t slot_connected[MAX(PERIPHERALS, 1)];
static bool blink_on;

static void blink_handler(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(blink_work, blink_handler);

// Blinks a generic red warning when the strip is too short to identify every peripheral.
// Otherwise, blinks the highest numbered missing required peripheral.
static void blink_handler(struct k_work *work) {
    if (!device_is_ready(strip)) {
        k_work_reschedule(&blink_work, K_MSEC(CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS));
        return;
    }

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
    // The layer indication has priority and restarts the blinking when it is done.
    if (layer_active) {
        return;
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)
    // The battery display restarts the blinking when it is done.
    if (battery_active) {
        return;
    }
#endif

    int missing = 0;
    for (int i = 0; i < PERIPHERALS; i++) {
        int number = slot_number(i);

        if (number <= REQUIRED && !atomic_get(&slot_connected[i])) {
            missing = MAX(missing, number);
        }
    }
    // Use a generic warning when the strip can't identify every peripheral.
    int lit = missing > 0 && STRIP_LEN < PERIPHERALS
                  ? STRIP_LEN
                  : missing;

    // If no essential peripherals are missing, the LEDs will be off.
    blink_on = missing > 0 && !blink_on;

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
    // With the layer always on, the layer is shown when the red LEDs are not.
    if (layer_stays_on() && !blink_on && leds_enabled) {
        draw_layer(false, FEEDBACK_NONE);
    } else
#endif
    {
        update_strip(blink_on ? lit : 0, (struct led_rgb){.r = SCALE(255), .g = 0, .b = 0},
                     FEEDBACK_CONNECTION_WARNING);
    }
    k_work_reschedule(&blink_work, K_MSEC(CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS));
}

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)

// Restarts the blinking work when the connection status changes.
static void connection_restart(void) {
    k_work_reschedule(&blink_work, K_MSEC(CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS));
}
#endif

// Sets the connection status for a peripheral slot based on the Bluetooth connection.
static void set_connected(struct bt_conn *conn, bool connected) {
    struct bt_conn_info info;

    if (bt_conn_get_info(conn, &info) < 0 || info.role != BT_CONN_ROLE_CENTRAL) {
        return;
    }

    int slot = zmk_ble_put_peripheral_addr(bt_conn_get_dst(conn));
    if (slot >= 0 && slot < PERIPHERALS) {
        atomic_set(&slot_connected[slot], connected);
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
    k_work_reschedule(&blink_work, K_MSEC(CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS));
    return 0;
}

// Initializes the LED feedback system for connection status. Schedules the initial blink work.
SYS_INIT(led_feedback_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif // CONFIG_ZMK_LED_FEEDBACK_CONNECTION

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_BATTERY)

// Last reported battery level (0-100) per peripheral slot. 0 means no reading:
// nothing reported yet, or the peripheral disconnected (the central reports 0 then).
static uint8_t battery_level[PERIPHERALS];

// The device numbers still to show, and whether the last step was a display (not a gap).
static int battery_number;
static int battery_last;
static bool battery_shown;

// A warning blinks BATTERY_WARNING_BLINKS times, each blink red then yellow,
// with a dark pause between the blinks. It is generic: at most WARNING_LEDS
// LEDs light up, whichever peripheral is low. warning_step counts the steps
// shown so far (red, yellow, pause).
static bool battery_warning;
static int warning_step;

#define WARNING_BLINKS CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS
#define WARNING_STEPS (WARNING_BLINKS * 3 - 1)
#define WARNING_LEDS MIN(STRIP_LEN, 4) // Use at most 4 LEDs to conserve energy.
#define WARNING_REPEAT (CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_REPEAT_MINUTES > 0)
#define WARNING_STEP_MS CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS
#define WARNING_PAUSE_MS 1000

#if CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL > 0
static bool battery_warned;
#endif

static void battery_handler(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(battery_work, battery_handler);
static void warning_handler(struct k_work *work);
static void show_warning(void);
static K_WORK_DELAYABLE_DEFINE(warning_work, warning_handler);

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

// Cancels any ongoing battery indication and warning work. Resets the battery state.
static void battery_cancel(void) {
#if CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL > 0
    if (battery_warning) {
        battery_warned = false;
    }
#endif
    battery_active = false;
    battery_warning = false;
    k_work_cancel_delayable(&battery_work);
    k_work_cancel_delayable(&warning_work);
}

// Brings back the layer indication (or clears the strip) and the connection blinking.
static void battery_finish(void) {
    battery_active = false;

#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
    // Clear the layer indication before finishing.
    layer_clear_handler(NULL);
#else
    // Clear the strip if layer feedback is not enabled.
    update_strip(0, (struct led_rgb){0}, FEEDBACK_NONE);
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_CONNECTION)
    // Resume connection blinking if enabled.
    k_work_reschedule(&blink_work, K_MSEC(CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS));
#endif
#if CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL > 0
    battery_warn_check();
#endif
#endif
}

// Blinks red then yellow, then brings back the normal display.
static void warning_handler(struct k_work *work) {
    if (warning_step >= WARNING_STEPS) {
        battery_warning = false;
        battery_finish();
        return;
    }

    int phase = warning_step % 3;

    if (phase == 2) {
        update_strip(0, (struct led_rgb){0}, FEEDBACK_BATTERY_WARNING);
    } else {
        update_strip(WARNING_LEDS, scale_rgb(phase ? 0xFFFF00 : 0xFF0000),
                     FEEDBACK_BATTERY_WARNING);
    }

    warning_step++;
    k_work_reschedule(&warning_work, K_MSEC(phase == 2 ? WARNING_PAUSE_MS : WARNING_STEP_MS));
}

// Shows one device at a time. The number of LEDs follows the charge and the color goes from
// green to yellow to red. A single blue LED means there is no reading.
static void battery_handler(struct k_work *work) {
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
    uint8_t level = slot >= 0 ? battery_level[slot] : 0;
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

#if CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL > 0
#define WARNING_REPEAT_MS (CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_REPEAT_MINUTES * 60 * 1000)
#define WARNING_RETRY_MS 10000

// Checks if any peripheral has a battery level below the warning threshold.
static bool any_battery_low(void) {
    for (int i = 0; i < PERIPHERALS; i++) {
        if (battery_level[i] > 0 &&
            battery_level[i] < CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL) {
            return true;
        }
    }

    return false;
}

#if WARNING_REPEAT
static void warning_repeat_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(warning_repeat_work, warning_repeat_handler);

// Warns again while a peripheral is still below the warning level.
// When the strip is busy, it tries again soon.
static void warning_repeat_handler(struct k_work *work) {
    if (!any_battery_low()) {
        return;
    }

    if (battery_active) {
        k_work_reschedule(&warning_repeat_work, K_MSEC(WARNING_RETRY_MS));
        return;
    }

    show_warning();
    k_work_reschedule(&warning_repeat_work, K_MSEC(WARNING_REPEAT_MS));
}
#endif

// Starts the warning when a peripheral drops below the warning level, and repeats it later.
static void battery_warn_check(void) {
    if (!any_battery_low()) {
        battery_warned = false;

        // Stop a warning that is still blinking.
        if (battery_warning) {
            battery_cancel();
            battery_finish();
        }
#if WARNING_REPEAT
        k_work_cancel_delayable(&warning_repeat_work);
#endif
        return;
    }

    // If any peripheral is below the warning level, start the warning sequence.
    if (battery_warned || battery_active) {
        return;
    }

    battery_warned = true;
    show_warning();
#if WARNING_REPEAT
    k_work_reschedule(&warning_repeat_work, K_MSEC(WARNING_REPEAT_MS));
#endif
}
#endif

// Listener for peripheral battery state changes. Updates the battery level and triggers warnings if necessary.
static int battery_listener(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);

    if (ev && ev->source < PERIPHERALS) {
        battery_level[ev->source] = ev->state_of_charge;

#if CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL > 0
        battery_warn_check();
#endif
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
    if (device_is_ready(strip)) {
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

    // Changing the brightness turns the LEDs on.
    leds_enabled = true;
    show_preview();
}

// Shows the current layer on the LED strip.
void zmk_led_feedback_show_layer(void) {
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
    if (device_is_ready(strip)) {
        layer_feedback_requested = true;
        k_work_reschedule(&layer_show_work, K_NO_WAIT);
    }
#endif
}

// Sets whether the layer indication should always be on.
void zmk_led_feedback_set_layer_always_on(int mode) {
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
    layer_always_on = mode < 0 ? !layer_always_on : mode > 0;
    if (device_is_ready(strip)) {
        k_work_reschedule(&layer_show_work, K_NO_WAIT);
    }
#endif
}

// Enables or disables the LED feedback.
void zmk_led_feedback_set_enabled(int mode) {
    leds_enabled = mode < 0 ? !leds_enabled : mode > 0;

    if (!device_is_ready(strip)) {
        return;
    }

    if (leds_enabled) {
        show_preview();
    } else {
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
#endif

    battery_active = true;
}

// Shows a battery warning on the LED strip.
static void show_warning(void) {
    if (!device_is_ready(strip) || !feedback_can_show(FEEDBACK_BATTERY_WARNING)) {
        return;
    }

    battery_take_over();
    battery_warning = true;
    warning_step = 0;
    k_work_reschedule(&warning_work, K_NO_WAIT);
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
