#pragma once

#include <stdbool.h>
#include <stdint.h>

#define LF_WARNING_MAX_PERIPHERALS 16
#define LF_WARNING_BOOT_MS 20000
#define LF_WARNING_NO_DEADLINE UINT64_MAX

enum lf_warning_output_type {
    LF_WARNING_OUTPUT_NONE,
    LF_WARNING_OUTPUT_CONNECTION,
    LF_WARNING_OUTPUT_BATTERY,
};

enum lf_warning_battery_status {
    LF_WARNING_BATTERY_NONE,
    LF_WARNING_BATTERY_PENDING,
    LF_WARNING_BATTERY_ACTIVE,
    LF_WARNING_BATTERY_DONE,
};

enum lf_warning_battery_phase {
    LF_WARNING_BATTERY_RED,
    LF_WARNING_BATTERY_YELLOW,
    LF_WARNING_BATTERY_PAUSE,
};

struct lf_warning_output {
    enum lf_warning_output_type type;
    uint16_t led_count;
    uint32_t color;
};

struct lf_warning_config {
    uint8_t peripheral_count;
    int8_t required_count;
    uint8_t device_numbers[LF_WARNING_MAX_PERIPHERALS];
    bool connection_enabled;
    uint8_t battery_threshold;
    uint8_t battery_blinks;
    uint16_t blink_interval_ms;
    uint16_t strip_length;
};

struct lf_warning_state {
    struct lf_warning_config config;
    uint64_t boot_deadline_ms;
    uint64_t phase_deadline_ms;
    uint32_t connected_slots;
    uint8_t battery_levels[LF_WARNING_MAX_PERIPHERALS];
    enum lf_warning_battery_status battery_status;
    enum lf_warning_battery_phase battery_phase;
    uint8_t battery_blinks_completed;
    bool boot_closed;
    bool display_enabled;
    bool connection_pending;
    bool connection_active;
    bool connection_red;
    bool connection_phase_pending;
    bool battery_started;
};

bool lf_warning_init(struct lf_warning_state *state, const struct lf_warning_config *config,
                     uint64_t now_ms);
void lf_warning_advance(struct lf_warning_state *state, uint64_t now_ms);
void lf_warning_connection_event(struct lf_warning_state *state, uint64_t now_ms, uint8_t slot,
                                 bool connected);
void lf_warning_battery_event(struct lf_warning_state *state, uint64_t now_ms, uint8_t slot,
                              uint8_t level);
void lf_warning_set_display(struct lf_warning_state *state, uint64_t now_ms, bool enabled);
bool lf_warning_render_connection_phase(struct lf_warning_state *state, uint64_t phase_start_ms);
uint64_t lf_warning_next_deadline(const struct lf_warning_state *state);
struct lf_warning_output lf_warning_get_connection_candidate(const struct lf_warning_state *state);
struct lf_warning_output lf_warning_get_output(const struct lf_warning_state *state);