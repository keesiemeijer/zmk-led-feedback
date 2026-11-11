#include <stdint.h>

int zmk_led_feedback_layer_led_count(int layer, int count) {
    return layer == 3 ? 100 : count;
}

uint32_t zmk_led_feedback_layer_color(int layer, int led_count, uint32_t color) {
    return layer == 3 && led_count == 4 ? 0x0000FF : color;
}