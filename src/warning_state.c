#include "warning_state.h"

#include <string.h>

#define LF_WARNING_RED 0xFF0000
#define LF_WARNING_YELLOW 0xFFFF00
#define LF_WARNING_BATTERY_PAUSE_MS 1000

static bool slot_required(const struct lf_warning_state *state, uint8_t slot) {
    int required = state->config.required_count < 0 ? state->config.peripheral_count
                                                    : state->config.required_count;
    uint8_t number = state->config.device_numbers[slot];
    return number > 0 && number <= required;
}

static bool valid_config(const struct lf_warning_config *config) {
    if (!config || config->peripheral_count > LF_WARNING_MAX_PERIPHERALS ||
        config->required_count < -1 || config->required_count > config->peripheral_count ||
        !config->strip_length || config->blink_interval_ms < 100 ||
        config->blink_interval_ms > 1000 || config->battery_blinks < 1 ||
        config->battery_blinks > 10 || config->battery_threshold == 1 ||
        config->battery_threshold > 100) {
        return false;
    }

    uint32_t numbers = 0;
    for (uint8_t slot = 0; slot < config->peripheral_count; slot++) {
        uint8_t number = config->device_numbers[slot];
        if (!number || number > config->peripheral_count) {
            return false;
        }
        uint32_t bit = UINT32_C(1) << (number - 1);
        if (numbers & bit) {
            return false;
        }
        numbers |= bit;
    }
    return true;
}

static uint32_t missing_slots(const struct lf_warning_state *state) {
    uint32_t missing = 0;
    for (uint8_t slot = 0; slot < state->config.peripheral_count; slot++) {
        uint32_t bit = UINT32_C(1) << slot;
        if (slot_required(state, slot) && !(state->connected_slots & bit)) {
            missing |= bit;
        }
    }
    return missing;
}

static struct lf_warning_output connection_output(const struct lf_warning_state *state,
                                                  bool red) {
    struct lf_warning_output output = {.type = LF_WARNING_OUTPUT_CONNECTION};
    if (!red) {
        return output;
    }

    uint32_t missing = missing_slots(state);
    uint8_t highest_number = 0;
    for (uint8_t slot = 0; slot < state->config.peripheral_count; slot++) {
        if ((missing & (UINT32_C(1) << slot)) &&
            state->config.device_numbers[slot] > highest_number) {
            highest_number = state->config.device_numbers[slot];
        }
    }
    output.led_count = state->config.strip_length < state->config.peripheral_count
                           ? state->config.strip_length
                           : highest_number;
    output.color = LF_WARNING_RED;
    return output;
}

static void start_battery(struct lf_warning_state *state, uint64_t now_ms) {
    state->battery_status = LF_WARNING_BATTERY_ACTIVE;
    state->battery_started = true;
    state->battery_phase = LF_WARNING_BATTERY_RED;
    state->battery_blinks_completed = 0;
    state->phase_deadline_ms = now_ms + state->config.blink_interval_ms;
}

static void finish_connection(struct lf_warning_state *state, uint64_t now_ms) {
    state->connection_pending = false;
    state->connection_active = false;
    state->connection_red = false;
    state->connection_phase_pending = false;
    state->phase_deadline_ms = LF_WARNING_NO_DEADLINE;
    if (state->display_enabled && state->battery_status == LF_WARNING_BATTERY_PENDING) {
        start_battery(state, now_ms);
    }
}

static void queue_connection(struct lf_warning_state *state) {
    if (!state->connection_active) {
        state->connection_pending = true;
    }
}

static void reconcile_boot(struct lf_warning_state *state, uint64_t now_ms) {
    if (state->boot_closed || !state->display_enabled) {
        return;
    }

    bool connection_needed = state->config.connection_enabled && missing_slots(state);
    if (connection_needed) {
        queue_connection(state);
    } else if (!connection_needed && state->connection_active) {
        finish_connection(state, now_ms);
    } else {
        state->connection_pending = false;
    }
    if (!connection_needed && !state->connection_active &&
        state->battery_status == LF_WARNING_BATTERY_PENDING) {
        start_battery(state, now_ms);
    }
}

bool lf_warning_init(struct lf_warning_state *state, const struct lf_warning_config *config,
                     uint64_t now_ms) {
    memset(state, 0, sizeof(*state));
    state->boot_closed = true;
    state->phase_deadline_ms = LF_WARNING_NO_DEADLINE;
    if (!valid_config(config)) {
        return false;
    }

    state->config = *config;
    state->boot_deadline_ms = now_ms + LF_WARNING_BOOT_MS;
    state->boot_closed = false;
    state->display_enabled = true;
    reconcile_boot(state, now_ms);
    return true;
}

void lf_warning_advance(struct lf_warning_state *state, uint64_t now_ms) {
    if (!state->boot_closed && now_ms >= state->boot_deadline_ms) {
        state->boot_closed = true;
        state->connection_pending = false;
        if (!state->display_enabled && state->battery_status == LF_WARNING_BATTERY_PENDING) {
            state->battery_status = LF_WARNING_BATTERY_DONE;
        }
        if (state->connection_active) {
            finish_connection(state, now_ms);
        } else if (state->display_enabled &&
                   state->battery_status == LF_WARNING_BATTERY_PENDING) {
            start_battery(state, now_ms);
        }
    }

    if (now_ms < state->phase_deadline_ms) {
        return;
    }

    if (state->connection_active) {
        if (state->connection_red) {
            state->connection_red = false;
            state->phase_deadline_ms = now_ms + state->config.blink_interval_ms;
        } else {
            state->connection_phase_pending = true;
            state->phase_deadline_ms = LF_WARNING_NO_DEADLINE;
        }
    } else if (state->battery_status == LF_WARNING_BATTERY_ACTIVE) {
        if (state->battery_phase == LF_WARNING_BATTERY_RED) {
            state->battery_phase = LF_WARNING_BATTERY_YELLOW;
            state->phase_deadline_ms = now_ms + state->config.blink_interval_ms;
        } else if (state->battery_phase == LF_WARNING_BATTERY_YELLOW) {
            state->battery_blinks_completed++;
            if (state->battery_blinks_completed >= state->config.battery_blinks) {
                state->battery_status = LF_WARNING_BATTERY_DONE;
                state->phase_deadline_ms = LF_WARNING_NO_DEADLINE;
            } else {
                state->battery_phase = LF_WARNING_BATTERY_PAUSE;
                state->phase_deadline_ms = now_ms + LF_WARNING_BATTERY_PAUSE_MS;
            }
        } else {
            state->battery_phase = LF_WARNING_BATTERY_RED;
            state->phase_deadline_ms = now_ms + state->config.blink_interval_ms;
        }
    }
}

bool lf_warning_render_connection_phase(struct lf_warning_state *state, uint64_t phase_start_ms) {
    lf_warning_advance(state, phase_start_ms);

    bool phase_pending = state->connection_pending ||
                         (state->connection_active && state->connection_phase_pending);
    if (!phase_pending) {
        return false;
    }

    bool fits_before_deadline =
        !state->boot_closed && phase_start_ms <= state->boot_deadline_ms &&
        state->config.blink_interval_ms <= state->boot_deadline_ms - phase_start_ms;
    if (!state->display_enabled || !fits_before_deadline || !missing_slots(state)) {
        if (state->connection_active) {
            finish_connection(state, phase_start_ms);
        } else {
            state->connection_pending = false;
            if (state->display_enabled &&
                state->battery_status == LF_WARNING_BATTERY_PENDING) {
                start_battery(state, phase_start_ms);
            }
        }
        return false;
    }

    if (!state->connection_active && state->battery_status == LF_WARNING_BATTERY_ACTIVE) {
        state->battery_status = LF_WARNING_BATTERY_PENDING;
    }
    state->connection_pending = false;
    state->connection_active = true;
    state->connection_red = true;
    state->connection_phase_pending = false;
    state->phase_deadline_ms = phase_start_ms + state->config.blink_interval_ms;
    return true;
}

void lf_warning_connection_event(struct lf_warning_state *state, uint64_t now_ms, uint8_t slot,
                                 bool connected) {
    lf_warning_advance(state, now_ms);
    if (slot >= state->config.peripheral_count || slot >= LF_WARNING_MAX_PERIPHERALS) {
        return;
    }

    uint32_t bit = UINT32_C(1) << slot;
    if (connected) {
        state->connected_slots |= bit;
    } else {
        state->connected_slots &= ~bit;
        state->battery_levels[slot] = 0;
    }
    reconcile_boot(state, now_ms);
}

void lf_warning_battery_event(struct lf_warning_state *state, uint64_t now_ms, uint8_t slot,
                              uint8_t level) {
    lf_warning_advance(state, now_ms);
    if (slot >= state->config.peripheral_count || slot >= LF_WARNING_MAX_PERIPHERALS) {
        return;
    }

    state->battery_levels[slot] = level;
    if (!state->boot_closed && slot_required(state, slot) && level > 0 &&
        level < state->config.battery_threshold &&
        state->battery_status == LF_WARNING_BATTERY_NONE) {
        state->battery_status = LF_WARNING_BATTERY_PENDING;
    }
    reconcile_boot(state, now_ms);
}

void lf_warning_set_display(struct lf_warning_state *state, uint64_t now_ms, bool enabled) {
    lf_warning_advance(state, now_ms);
    if (state->display_enabled == enabled) {
        return;
    }

    state->display_enabled = enabled;
    if (!enabled) {
        state->connection_pending = false;
        state->connection_active = false;
        state->connection_red = false;
        state->connection_phase_pending = false;
        state->phase_deadline_ms = LF_WARNING_NO_DEADLINE;
        if (state->battery_started && (state->battery_status == LF_WARNING_BATTERY_ACTIVE ||
                                       state->battery_status == LF_WARNING_BATTERY_PENDING)) {
            state->battery_status = LF_WARNING_BATTERY_DONE;
        } else if (state->boot_closed && state->battery_status == LF_WARNING_BATTERY_PENDING) {
            state->battery_status = LF_WARNING_BATTERY_DONE;
        }
    } else {
        reconcile_boot(state, now_ms);
    }
}

uint64_t lf_warning_next_deadline(const struct lf_warning_state *state) {
    if (!state->boot_closed && state->boot_deadline_ms < state->phase_deadline_ms) {
        return state->boot_deadline_ms;
    }
    return state->phase_deadline_ms;
}

struct lf_warning_output lf_warning_get_connection_candidate(const struct lf_warning_state *state) {
    struct lf_warning_output output = {0};
    if (!state->display_enabled || state->boot_closed ||
        !(state->connection_pending ||
          (state->connection_active && state->connection_phase_pending)) ||
        !missing_slots(state)) {
        return output;
    }
    return connection_output(state, true);
}

struct lf_warning_output lf_warning_get_output(const struct lf_warning_state *state) {
    struct lf_warning_output output = {0};
    if (!state->display_enabled) {
        return output;
    }

    if (state->connection_active) {
        return connection_output(state, state->connection_red);
    } else if (state->battery_status == LF_WARNING_BATTERY_ACTIVE) {
        output.type = LF_WARNING_OUTPUT_BATTERY;
        if (state->battery_phase != LF_WARNING_BATTERY_PAUSE) {
            output.led_count = state->config.strip_length < 4 ? state->config.strip_length : 4;
            output.color = state->battery_phase == LF_WARNING_BATTERY_RED ? LF_WARNING_RED
                                                                          : LF_WARNING_YELLOW;
        }
    }
    return output;
}