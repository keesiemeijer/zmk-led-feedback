#include <assert.h>
#include <stdint.h>

#include "startup_diagnostic.h"

static int64_t uptime_ms;

int64_t k_uptime_get(void) {
    return uptime_ms;
}

int main(void) {
    uptime_ms = 0;
    assert(startup_diagnostic_is_active());
    uptime_ms = 1;
    assert(startup_diagnostic_is_active());
    uptime_ms = 19999;
    assert(startup_diagnostic_is_active());
    uptime_ms = 20000;
    assert(!startup_diagnostic_is_active());
    uptime_ms = 20001;
    assert(!startup_diagnostic_is_active());
    uptime_ms = INT64_MAX;
    assert(!startup_diagnostic_is_active());

    struct startup_peripheral peripherals[] = {
        {.number = 2, .connected = false, .battery = 0},
        {.number = 1, .connected = false, .battery = 14},
        {.number = 3, .connected = true, .battery = 100},
    };
    int missing = -1;
    assert(startup_diagnostic_warning(peripherals, 3, 2, true, true, &missing) ==
           STARTUP_WARNING_CONNECTION);
    assert(missing == 2);
    peripherals[0].connected = true;
    peripherals[1].connected = true;
    assert(startup_diagnostic_warning(peripherals, 3, 2, true, true, &missing) ==
           STARTUP_WARNING_LOW_BATTERY);
    assert(missing == 0);
    peripherals[1].battery = 15;
    assert(startup_diagnostic_warning(peripherals, 3, 2, true, true, &missing) ==
           STARTUP_WARNING_MISSING_BATTERY);
    peripherals[0].battery = 100;
    assert(startup_diagnostic_warning(peripherals, 3, 2, true, true, &missing) ==
           STARTUP_WARNING_NONE);
    peripherals[2].battery = 14;
    assert(startup_diagnostic_warning(peripherals, 3, 0, true, true, &missing) ==
           STARTUP_WARNING_LOW_BATTERY);
    peripherals[2].battery = 100;
    peripherals[0].battery = 0;
    assert(startup_diagnostic_warning(peripherals, 3, 0, true, true, &missing) ==
           STARTUP_WARNING_NONE);
    assert(startup_diagnostic_warning(peripherals, 3, 1, true, true, &missing) ==
           STARTUP_WARNING_NONE);
    assert(startup_diagnostic_warning(peripherals, 3, 2, false, true, &missing) ==
           STARTUP_WARNING_MISSING_BATTERY);
    peripherals[0].connected = false;
    assert(startup_diagnostic_warning(peripherals, 3, 2, true, false, &missing) ==
           STARTUP_WARNING_CONNECTION);
    assert(startup_diagnostic_warning(peripherals, 3, 2, false, false, &missing) ==
           STARTUP_WARNING_NONE);

    struct startup_pattern pattern = {0};
    struct startup_frame frame = startup_diagnostic_frame(
        &pattern, STARTUP_WARNING_CONNECTION, 2, 0, 500);
    assert(frame.changed && frame.color == 0xFF0000 && frame.next_phase == 500);
    frame = startup_diagnostic_frame(&pattern, STARTUP_WARNING_CONNECTION, 2, 499, 500);
    assert(!frame.changed && frame.next_phase == 500);
    frame = startup_diagnostic_frame(&pattern, STARTUP_WARNING_CONNECTION, 2, 500, 500);
    assert(frame.changed && frame.color == 0 && frame.connection_gap);
    frame = startup_diagnostic_frame(&pattern, STARTUP_WARNING_MISSING_BATTERY, 0, 501, 500);
    assert(frame.changed && frame.color == 0xFFFF00 && frame.next_phase == 1001);
    frame = startup_diagnostic_frame(&pattern, STARTUP_WARNING_MISSING_BATTERY, 0, 1001, 500);
    assert(frame.changed && frame.color == 0 && !frame.connection_gap);
    frame = startup_diagnostic_frame(&pattern, STARTUP_WARNING_LOW_BATTERY, 0, 1002, 500);
    assert(frame.changed && frame.color == 0xFF0000 && frame.next_phase == 1502);
    frame = startup_diagnostic_frame(&pattern, STARTUP_WARNING_LOW_BATTERY, 0, 1502, 500);
    assert(frame.changed && frame.color == 0xFFFF00 && frame.next_phase == 2002);
    frame = startup_diagnostic_frame(&pattern, STARTUP_WARNING_LOW_BATTERY, 0, 2002, 500);
    assert(frame.changed && frame.color == 0 && frame.next_phase == 3002);
    frame = startup_diagnostic_frame(&pattern, STARTUP_WARNING_LOW_BATTERY, 0, 3002, 500);
    assert(frame.changed && frame.color == 0xFF0000 && frame.next_phase == 3502);
    pattern.warning = STARTUP_WARNING_NONE;
    frame = startup_diagnostic_frame(&pattern, STARTUP_WARNING_LOW_BATTERY, 0, 3003, 500);
    assert(frame.changed && frame.color == 0xFF0000);
    return 0;
}
