/*
 * Copyright (c) 2026 keesiemeijer
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include "startup_diagnostic.h"

bool startup_diagnostic_is_active(void) {
    return k_uptime_get() < STARTUP_DIAGNOSTIC_DURATION_MS;
}

enum startup_warning startup_diagnostic_warning(const struct startup_peripheral *peripherals,
                                                size_t count, int required,
                                                bool connection_enabled, bool battery_enabled,
                                                int *missing_connection) {
    int missing = 0;
    bool low = false;
    bool missing_battery = false;

    for (size_t i = 0; i < count; i++) {
        const struct startup_peripheral *peripheral = &peripherals[i];
        bool is_required = peripheral->number <= required;
        if (connection_enabled && is_required && !peripheral->connected &&
            peripheral->number > missing) {
            missing = peripheral->number;
        }
        if (battery_enabled) {
            low |= peripheral->battery > 0 && peripheral->battery < 15;
            missing_battery |= is_required && peripheral->battery == 0;
        }
    }

    *missing_connection = missing;
    if (missing > 0) {
        return STARTUP_WARNING_CONNECTION;
    }
    if (low) {
        return STARTUP_WARNING_LOW_BATTERY;
    }
    if (missing_battery) {
        return STARTUP_WARNING_MISSING_BATTERY;
    }
    return STARTUP_WARNING_NONE;
}

struct startup_frame startup_diagnostic_frame(struct startup_pattern *pattern,
                                              enum startup_warning warning, int missing_connection,
                                              int64_t now, int interval_ms) {
    struct startup_frame frame = {0};
    if (warning != pattern->warning ||
        (warning == STARTUP_WARNING_CONNECTION && missing_connection != pattern->missing_connection)) {
        pattern->warning = warning;
        pattern->missing_connection = missing_connection;
        pattern->phase = 0;
        pattern->next_phase = now;
    }

    if (warning == STARTUP_WARNING_NONE || now < pattern->next_phase) {
        frame.next_phase = pattern->next_phase;
        return frame;
    }

    frame.changed = true;
    int duration = interval_ms;
    if (warning == STARTUP_WARNING_CONNECTION) {
        frame.connection_gap = pattern->phase != 0;
        frame.color = pattern->phase ? 0 : 0xFF0000;
        pattern->phase = 1 - pattern->phase;
    } else if (warning == STARTUP_WARNING_MISSING_BATTERY) {
        frame.color = pattern->phase ? 0 : 0xFFFF00;
        pattern->phase = 1 - pattern->phase;
    } else {
        frame.color = pattern->phase == 0 ? 0xFF0000 : pattern->phase == 1 ? 0xFFFF00 : 0;
        if (pattern->phase == 2) {
            duration = 1000;
        }
        pattern->phase = (pattern->phase + 1) % 3;
    }
    pattern->next_phase = now + duration;
    frame.next_phase = pattern->next_phase;
    return frame;
}
