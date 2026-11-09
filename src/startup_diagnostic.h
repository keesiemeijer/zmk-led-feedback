/*
 * Copyright (c) 2026 keesiemeijer
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STARTUP_DIAGNOSTIC_DURATION_MS 20000

bool startup_diagnostic_is_active(void);
bool startup_diagnostic_display_enabled(bool configured_enabled, bool explicitly_set, bool active);
bool startup_diagnostic_set_display(bool configured_enabled, bool explicitly_set, bool active,
                                    int mode);

enum startup_warning {
    STARTUP_WARNING_NONE,
    STARTUP_WARNING_CONNECTION,
    STARTUP_WARNING_LOW_BATTERY,
    STARTUP_WARNING_MISSING_BATTERY,
};

struct startup_peripheral {
    int number;
    bool connected;
    uint8_t battery;
};

enum startup_warning startup_diagnostic_warning(const struct startup_peripheral *peripherals,
                                                size_t count, int required,
                                                bool connection_enabled, bool battery_enabled,
                                                int *missing_connection);

struct startup_pattern {
    enum startup_warning warning;
    int missing_connection;
    int phase;
    int64_t next_phase;
};

struct startup_frame {
    bool changed;
    bool connection_gap;
    uint32_t color;
    int64_t next_phase;
};

struct startup_frame startup_diagnostic_frame(struct startup_pattern *pattern,
                                              enum startup_warning warning, int missing_connection,
                                              int64_t now, int interval_ms);
