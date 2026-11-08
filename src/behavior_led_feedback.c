/*
 * Copyright (c) 2026 keesiemeijer
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_led_feedback

#include <drivers/behavior.h>
#include <zephyr/device.h>

#include <dt-bindings/zmk/led_feedback.h>
#include <zmk/behavior.h>
#include <zmk_led_feedback.h>

// LED feedback behavior implementation for ZMK.
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

// Handler for when a behavior binding is pressed.
static int on_binding_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case LF_BRI_INC:
        zmk_led_feedback_change_brightness(1);
        break;
    case LF_BRI_DEC:
        zmk_led_feedback_change_brightness(-1);
        break;
    case LF_TOG:
        zmk_led_feedback_set_enabled(-1);
        break;
    case LF_ON:
        zmk_led_feedback_set_enabled(1);
        break;
    case LF_OFF:
        zmk_led_feedback_set_enabled(0);
        break;
    case LF_LAYER:
        zmk_led_feedback_show_layer();
        break;
    case LF_LAYER_TOG:
        zmk_led_feedback_set_layer_always_on(-1);
        break;
    default:
        return -ENOTSUP;
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

// Handler for when a behavior binding is released.
static int on_binding_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

// LED feedback behavior driver API structure.
static const struct behavior_driver_api led_feedback_behavior_api = {
    .binding_pressed = on_binding_pressed,
    .binding_released = on_binding_released,
};

// Register the LED feedback behavior with ZMK.
// Define the LED feedback behavior for the device tree instance 0.
BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &led_feedback_behavior_api);

#endif
