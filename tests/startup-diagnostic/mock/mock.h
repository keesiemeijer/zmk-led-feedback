#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

#define CONFIG_ZMK_LED_FEEDBACK 1
#define CONFIG_ZMK_LED_FEEDBACK_STARTUP_DIAGNOSTIC 1
#define CONFIG_ZMK_LED_FEEDBACK_LAYER 1
#define CONFIG_ZMK_LED_FEEDBACK_CONNECTION 1
#define CONFIG_ZMK_LED_FEEDBACK_BATTERY 1
#define CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS 2
#define CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS -1
#define CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER ""
#define CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS 1
#define CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS 255
#define CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS 64
#define CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS_STEP 16
#define CONFIG_ZMK_LED_FEEDBACK_DISPLAY_OFF 1
#define CONFIG_ZMK_LED_FEEDBACK_REVERSE 0
#define CONFIG_ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON 0
#define CONFIG_ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON_LAYERS ""
#define CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS "00FF00"
#define CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER ""
#define CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT 0
#define CONFIG_ZMK_LED_FEEDBACK_LAYER_DURATION_MS 1000
#define CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS "00FF00,FFFF00,FF0000,0000FF"
#ifndef CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL
#define CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL 15
#endif
#define CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS 5
#define CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_REPEAT_MINUTES 1
#define CONFIG_ZMK_LED_FEEDBACK_BATTERY_DURATION_MS 1000
#define CONFIG_ZMK_LED_FEEDBACK_BATTERY_DELAY_MS 500
#define CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS 500
#ifndef CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES
#define CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES ""
#endif

#define IS_ENABLED(x) (x)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(v, lo, hi) MIN(MAX(v, lo), hi)
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define BUILD_ASSERT(condition, message) _Static_assert(condition, message)
#define DT_CHOSEN(x) 0
#define DT_PROP(node, prop) 4
#define DEVICE_DT_GET(node) (&mock_device)
#define __weak __attribute__((weak))
#define SYS_INIT(fn, level, priority)
#define ZMK_LISTENER(name, fn)
#define ZMK_SUBSCRIPTION(name, event)
#define ZMK_EV_EVENT_BUBBLE 0
#define BT_CONN_ROLE_CENTRAL 0

struct device { int unused; };
extern const struct device mock_device;
struct led_rgb { uint8_t r, g, b; };
bool device_is_ready(const struct device *device);
int led_strip_update_rgb(const struct device *device, struct led_rgb *pixels, size_t count);

typedef int atomic_t;
static inline int atomic_get(const atomic_t *value) { return *value; }
static inline void atomic_set(atomic_t *value, int next) { *value = next; }

struct k_mutex { pthread_mutex_t mutex; };
#define K_MUTEX_DEFINE(name) struct k_mutex name = { PTHREAD_MUTEX_INITIALIZER }
#define K_FOREVER 0
static inline int k_mutex_lock(struct k_mutex *mutex, int timeout) {
    return pthread_mutex_lock(&mutex->mutex);
}
static inline int k_mutex_unlock(struct k_mutex *mutex) {
    return pthread_mutex_unlock(&mutex->mutex);
}
struct k_work { int unused; };
struct k_work_delayable { bool scheduled; int64_t delay; };
#define K_WORK_DELAYABLE_DEFINE(name, handler) struct k_work_delayable name
#define K_MSEC(ms) (ms)
#define K_NO_WAIT 0
int64_t k_uptime_get(void);
static inline int k_work_reschedule(struct k_work_delayable *work, int64_t delay) {
    work->scheduled = true;
    work->delay = delay;
    return 0;
}
static inline int k_work_cancel_delayable(struct k_work_delayable *work) {
    work->scheduled = false;
    return 0;
}

struct bt_conn { int unused; };
struct bt_conn_info { int role; };
struct bt_conn_cb {
    void (*connected)(struct bt_conn *, uint8_t);
    void (*disconnected)(struct bt_conn *, uint8_t);
};
#define BT_CONN_CB_DEFINE(name) struct bt_conn_cb name
static inline int bt_conn_get_info(struct bt_conn *conn, struct bt_conn_info *info) {
    info->role = BT_CONN_ROLE_CENTRAL;
    return 0;
}
static inline const void *bt_conn_get_dst(struct bt_conn *conn) { return NULL; }
static inline int zmk_ble_put_peripheral_addr(const void *addr) { return 0; }

typedef void zmk_event_t;
struct zmk_peripheral_battery_state_changed { int source; uint8_t state_of_charge; };
static inline const struct zmk_peripheral_battery_state_changed *
as_zmk_peripheral_battery_state_changed(const zmk_event_t *event) { return event; }
int zmk_keymap_highest_layer_active(void);
