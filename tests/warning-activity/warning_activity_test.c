#include <assert.h>
#include "../../src/warning_activity.h"

int main(void) {
    const int64_t five_minutes = 300000;
    struct warning_activity activity = {0};

    assert(!warning_activity_press(&activity, 299999, five_minutes));
    assert(!warning_activity_press(&activity, 599998, five_minutes));
    assert(warning_activity_press(&activity, 899998, five_minutes));
    assert(!warning_activity_press(&activity, 899998, five_minutes));
    assert(!warning_activity_press(&activity, 600000, five_minutes));
    assert(warning_activity_press(&activity, 1199998, five_minutes));

    struct warning_activity no_presses = {0};
    assert(warning_activity_press(&no_presses, five_minutes, five_minutes));

    const int64_t one_day = 1440LL * 60 * 1000;
    struct warning_activity long_interval = {0};
    assert(!warning_activity_press(&long_interval, one_day - 1, one_day));
    assert(warning_activity_press(&long_interval, 2 * one_day - 1, one_day));
    return 0;
}
