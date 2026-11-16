#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define CONFIG_ZMK_LED_FEEDBACK 1
#ifndef CONFIG_ZMK_LED_FEEDBACK_CONNECTION
#define CONFIG_ZMK_LED_FEEDBACK_CONNECTION 1
#endif
#define CONFIG_ZMK_LED_FEEDBACK_BATTERY 1
#define CONFIG_ZMK_LED_FEEDBACK_LAYER 1
#define CONFIG_ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON 0
#define CONFIG_ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON_LAYERS ""
#define CONFIG_ZMK_LED_FEEDBACK_LAYER_DURATION_MS 1000
#ifndef CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES
#define CONFIG_ZMK_LED_FEEDBACK_LAYER_INACTIVITY_TIMEOUT_MINUTES 1
#endif
#define CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS "00FF00,FFFF00"
#define CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER ""
#define CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT 0
#ifndef CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS
#define CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS -1
#endif
#define CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL 20
#define CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS 5
#define CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS "00FF00,FFFF00,FF0000,0000FF"
#define CONFIG_ZMK_LED_FEEDBACK_BATTERY_DELAY_MS 500
#define CONFIG_ZMK_LED_FEEDBACK_BATTERY_DURATION_MS 1000
#define CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS 500
#define CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS 64
#define CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS 10
#define CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS 128
#define CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS_STEP 16
#define CONFIG_ZMK_LED_FEEDBACK_REVERSE 0
#define CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS 2
#ifndef CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER
#define CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER "1,2"
#endif
#define IS_ENABLED(value) (value)
#define DT_CHOSEN(node) 0
#define DT_PROP(node, property) 4
#define DEVICE_DT_GET(node) (&fake_device)
#define BUILD_ASSERT(condition, message) _Static_assert(condition, message)
#define MIN(left, right) ((left) < (right) ? (left) : (right))
#define MAX(left, right) ((left) > (right) ? (left) : (right))
#define CLAMP(value, low, high) MIN(MAX((value), (low)), (high))
#define DIV_ROUND_UP(value, divisor) (((value) + (divisor) - 1) / (divisor))
#define K_MSEC(value) (value)
#define K_NO_WAIT 0
#define K_FOREVER 0
#define ATOMIC_INIT(value) (value)
#define __weak __attribute__((weak))
#define SYS_INIT(function, level, priority)
#define ZMK_LISTENER(name, function)
#define ZMK_SUBSCRIPTION(name, event)
#define ZMK_EV_EVENT_BUBBLE 0
#define BT_CONN_ROLE_CENTRAL 1

typedef int atomic_t;
typedef int k_spinlock_key_t;

struct k_spinlock {
    int unused;
};

struct k_mutex {
    int unused;
};

#define K_MUTEX_DEFINE(name) struct k_mutex name

struct k_work {
    void (*handler)(struct k_work *work);
};

struct k_work_delayable {
    struct k_work work;
    bool pending;
    uint64_t deadline;
};

#define K_WORK_DELAYABLE_DEFINE(name, function) \
    struct k_work_delayable name = {.work = {.handler = function}}

struct device {
    bool ready;
};

struct led_rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct bt_conn {
    uint8_t slot;
};

struct bt_conn_info {
    int role;
};

struct bt_conn_cb {
    void (*connected)(struct bt_conn *conn, uint8_t error);
    void (*disconnected)(struct bt_conn *conn, uint8_t reason);
};

#define BT_CONN_CB_DEFINE(name) struct bt_conn_cb name

struct zmk_peripheral_battery_state_changed {
    uint8_t source;
    uint8_t state_of_charge;
};

typedef struct zmk_peripheral_battery_state_changed zmk_event_t;

struct zmk_position_state_changed {
    uint8_t source;
    uint32_t position;
    bool state;
    int64_t timestamp;
};

extern uint64_t fake_now_ms;
extern struct device fake_device;
extern struct led_rgb fake_pixels[4];
extern int fake_layer;

static inline int atomic_get(const atomic_t *value) { return *value; }
static inline void atomic_set(atomic_t *value, int updated) { *value = updated; }
static inline k_spinlock_key_t k_spin_lock(struct k_spinlock *lock) { return 0; }
static inline void k_spin_unlock(struct k_spinlock *lock, k_spinlock_key_t key) {}
static inline int k_mutex_lock(struct k_mutex *mutex, int timeout) { return 0; }
static inline int k_mutex_unlock(struct k_mutex *mutex) { return 0; }
static inline uint64_t k_uptime_get(void) { return fake_now_ms; }
static inline bool device_is_ready(const struct device *device) { return device->ready; }
static inline int zmk_keymap_highest_layer_active(void) { return fake_layer; }
static inline int bt_conn_get_info(struct bt_conn *conn, struct bt_conn_info *info) {
    info->role = BT_CONN_ROLE_CENTRAL;
    return 0;
}
static inline const uint8_t *bt_conn_get_dst(struct bt_conn *conn) { return &conn->slot; }
static inline int zmk_ble_put_peripheral_addr(const uint8_t *address) { return *address; }
static inline const struct zmk_peripheral_battery_state_changed *
as_zmk_peripheral_battery_state_changed(const zmk_event_t *event) {
    return event;
}
static inline const struct zmk_position_state_changed *
as_zmk_position_state_changed(const zmk_event_t *event) {
    return (const struct zmk_position_state_changed *)event;
}
static inline int k_work_reschedule(struct k_work_delayable *work, uint64_t delay) {
    work->pending = true;
    work->deadline = fake_now_ms + delay;
    return 0;
}
static inline int k_work_schedule(struct k_work_delayable *work, uint64_t delay) {
    if (!work->pending) {
        work->pending = true;
        work->deadline = fake_now_ms + delay;
    }
    return 0;
}
static inline int k_work_cancel_delayable(struct k_work_delayable *work) {
    work->pending = false;
    return 0;
}
static inline int led_strip_update_rgb(const struct device *device, struct led_rgb *pixels,
                                       size_t count) {
    memcpy(fake_pixels, pixels, count * sizeof(*pixels));
    return 0;
}