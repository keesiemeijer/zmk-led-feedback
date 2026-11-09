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
    return 0;
}
