#include <assert.h>
#include <string.h>

#include "mock.h"
#include "../../src/led_feedback.c"

const struct device mock_device = {0};
static struct led_rgb last_pixels[4];

int64_t k_uptime_get(void) { return 100; }
bool device_is_ready(const struct device *device) { return true; }
int zmk_keymap_highest_layer_active(void) { return 0; }
int led_strip_update_rgb(const struct device *device, struct led_rgb *values, size_t count) {
    assert(count == 4);
    memcpy(last_pixels, values, sizeof(last_pixels));
    return 0;
}

int main(void) {
    assert(!display_enabled());
    zmk_led_feedback_set_enabled(1);
    assert(display_enabled());
    slot_connected[0] = 1;
    slot_connected[1] = 0;
    blink_handler(NULL);
    assert(last_pixels[0].r > 0 && last_pixels[1].r > 0);
    slot_connected[1] = 1;
    blink_handler(NULL);
    assert(last_pixels[0].r == 0 && last_pixels[1].r == 0);
    zmk_led_feedback_set_enabled(0);
    assert(!display_enabled());
    return 0;
}
