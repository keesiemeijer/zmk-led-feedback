/*
 * Copyright (c) 2026 keesiemeijer
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>

// LED feedback control functions
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK)
// Changes the brightness by one step up (direction > 0) or down (direction < 0). Also turns the LEDs on.
void zmk_led_feedback_change_brightness(int direction);
// Turns the LEDs on (mode > 0), off (mode == 0) or toggles them (mode < 0).
void zmk_led_feedback_set_enabled(int mode);
// Shows the current layer for a short time (does nothing while the LEDs are off).
void zmk_led_feedback_show_layer(void);
// Shows the battery level of a peripheral for a short time (does nothing while the LEDs are off).
// The number is the device number from ZMK_LED_FEEDBACK_SLOT_ORDER. 0 shows all devices.
void zmk_led_feedback_show_battery(int number);
// Turns the LEDs on (mode > 0), off (mode == 0) or toggles them (mode < 0).
void zmk_led_feedback_set_layer_always_on(int mode);
#else
static inline void zmk_led_feedback_change_brightness(int direction) {}
static inline void zmk_led_feedback_set_enabled(int mode) {}
static inline void zmk_led_feedback_show_layer(void) {}
static inline void zmk_led_feedback_show_battery(int number) {}
static inline void zmk_led_feedback_set_layer_always_on(int mode) {}
#endif

// Optional hook. Define this function in your own code to change the number of LEDs for a layer indication.
int zmk_led_feedback_layer_led_count(int layer, int count);

// Optional hook. Define this function to change the color (0xRRGGBB) of a layer indication.
// It is called after the zmk_led_feedback_layer_led_count() hook, with the final number of LEDs, the layer and color.
uint32_t zmk_led_feedback_layer_color(int layer, int led_count, uint32_t color);

// Helpers for the hooks
#if IS_ENABLED(CONFIG_ZMK_LED_FEEDBACK_LAYER)
// Returns the number of LEDs in the strip.
int zmk_led_feedback_strip_length(void);
// Returns the color (0xRRGGBB) at an index in CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS (index 0 is the first color, the list repeats).
uint32_t zmk_led_feedback_palette_color(int index);
#endif
