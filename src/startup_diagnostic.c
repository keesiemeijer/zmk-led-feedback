/*
 * Copyright (c) 2026 keesiemeijer
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include "startup_diagnostic.h"

#define STARTUP_DIAGNOSTIC_DURATION_MS 20000

bool startup_diagnostic_is_active(void) {
    return k_uptime_get() < STARTUP_DIAGNOSTIC_DURATION_MS;
}
