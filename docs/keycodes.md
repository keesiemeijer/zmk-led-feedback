# Keycodes

This module provides two behaviors for controlling LED feedback. Add the behavior(s) to your zmk-config `.keymap` file, then use the corresponding bindings in a layer. None of the settings changed with keycodes is saved across restarts.

## LED feedback behavior

Include the LED feedback keycodes and declare the behavior in your keymap:

```dts
// LED feedback keycodes
#include <dt-bindings/zmk/led_feedback.h>

/ {
    behaviors {
        led_fb: led_feedback {
            compatible = "zmk,behavior-led-feedback";
            #binding-cells = <1>;
        };
    };
};
```

### General Keycodes

Keycodes used by the `behavior-led-feedback` behavior. (See above)

| Binding        | Action                                                                   |
| -------------- | ------------------------------------------------------------------------ |
| `LF_BRI_INC`   | Increase brightness by one step. Shows a preview by turning all LEDs on. |
| `LF_BRI_DEC`   | Decrease brightness by one step. Shows a preview by turning all LEDs on. |
| `LF_TOG`       | Toggle the LED display on or off.                                        |
| `LF_ON`        | Turn the LED display on                                                  |
| `LF_OFF`       | Turn the LED display off.                                                |
| `LF_LAYER`     | Show the current layer briefly.                                          |
| `LF_LAYER_TOG` | Toggle continuous layer indication                                       |

Brightness changes are limited by [`CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS`](../README.md#configuration-options) and
[`CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS`](../README.md#configuration-options).

#### Display and warning priority

The display starts on. When it is off, all feedback is suppressed; there are no display-off
overrides. Brightness keys turn the display on. If a boot warning is eligible, it is shown instead
of the brightness preview.

Connection and battery warnings take priority over layer feedback and on-demand battery displays.
Layer changes still take effect, and brightness or continuous-layer settings still update, but
feedback requests made during a warning are discarded rather than queued. Continuous layer
indication resumes with the current layer after warnings end if it is enabled. See the
[display behavior guide](display-off.md) for cancellation details.

#### Keymap

Use the key bindings with `&led_fb`

Example bindings in a layer:

```dts
&led_fb LF_BRI_INC  &led_fb LF_BRI_DEC  &led_fb LF_TOG
&led_fb LF_ON       &led_fb LF_OFF      &led_fb LF_LAYER_TOG
&led_fb LF_LAYER
```

## Led feedback battery behavior

Enable [battery feedback](battery-feedback.md#enabling-battery-feedback), then add the battery feedback behavior to your zmk-config `.keymap` file. It's assumed the LED keycodes are included at the top of the file ([see above](#led-feedback-behavior))

```dts
/ {
    behaviors {
        led_bat: led_feedback_battery {
            compatible = "zmk,behavior-led-feedback-battery";
            #binding-cells = <1>;
        };
    };
};
```

### Battery feedback Keycodes

Keycodes used by the `behavior-led-feedback-battery` behavior. (See above)

Use `&led_bat <device number>` to display a peripheral's battery:

| Binding      | Action                                                                             |
| ------------ | ---------------------------------------------------------------------------------- |
| `&led_bat 0` | Show all peripheral battery levels, one after another.                             |
| `&led_bat 1` | Show the battery level of peripheral device 1.                                     |
| `&led_bat N` | Show the battery level for device `N`, up to the configured number of peripherals. |

Device numbers follow [pairing order](peripheral-numbering.md#device-numbers-and-pairing-slots) unless changed with [`CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER`](peripheral-numbering.md#changing-the-pairing-order).

#### Display and warning priority

Battery requests are suppressed while the display is off or a warning is active. The shared
required-peripheral filter applies only to automatic warnings; on-demand battery requests can
still inspect optional peripherals when the display is on and no warning is active. A missing or
zero battery reading displays one LED in the configured no-reading color (blue by default).
Requests made during a warning are discarded and are not replayed afterward.

#### Keymap

Use `&led_bat <device number>` to display a peripheral's battery:

Example:

```dts
&led_bat 0  &led_bat 1  &led_bat 2
```
