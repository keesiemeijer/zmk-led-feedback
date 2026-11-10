/*
 * Copyright (c) 2026 keesiemeijer
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

struct warning_activity {
    int64_t last_press;
};

static inline bool warning_activity_press(struct warning_activity *activity, int64_t timestamp,
                                          int64_t inactivity_ms) {
    if (timestamp <= activity->last_press) {
        return false;
    }

    bool reset = timestamp - activity->last_press >= inactivity_ms;
    activity->last_press = timestamp;
    return reset;
}
