# Display-off behavior

The LED feedback display can be [turned off with keycodes](#led-feedback-display-keycodes), or [configured to start with the LED display turned off](#configuration-options)

While the LED display is off, feedback is suppressed unless you explicitly [allow selected feedback types](#override-feedback-types) with the `CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES` option below.

## Configuration options

Options to start with the LED feedback display turned off, and to provide overrides when the display is turned off.

| Option                                  | Default | Description                                                                                                                                                          |
| --------------------------------------- | ------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `CONFIG_ZMK_LED_FEEDBACK_DISPLAY_OFF`   | `n`     | Start with the display off after each boot.                                                                                                                          |
| `CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES` | `""`    | Comma-separated list of [feedback types](#override-feedback-types) allowed to show feedback temporarily while the display is off. Empty means no feedback overrides. |

### Examples

Set these options in your zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle. For example, start with the display off and allow only connection and battery warnings to show:

```ini
CONFIG_ZMK_LED_FEEDBACK_DISPLAY_OFF=y
CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES="battery_warning,connection_warning"
```

To allow every supported feedback type while the display is off:

```ini
CONFIG_ZMK_LED_FEEDBACK_DISPLAY_OFF=y
CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES="layer_feedback,layer_change,battery_feedback,battery_warning,connection_warning"
```

### Override feedback types

Override feedback types used by the [`CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES`](#configuration-options) option

The override list accepts these values:

| Value                | Feedback allowed while display is off                                                                                 |
| -------------------- | --------------------------------------------------------------------------------------------------------------------- |
| `layer_feedback`     | The current layer when requested with the [`LF_LAYER`](keycodes.md#general-keycodes) keycode.                         |
| `layer_change`       | Layer feedback after a ZMK [layer change keycode](https://zmk.dev/docs/keymaps/behaviors#layer-navigation-behaviors). |
| `battery_feedback`   | Battery levels requested with the [`&led_bat N`](keycodes.md#battery-feedback-keycodes) keycode.                      |
| `battery_warning`    | [Low-battery warnings](battery-feedback.md#low-battery-warnings) warnings.                                            |
| `connection_warning` | [Connection warnings](connection-feedback.md#connection-warning) when required peripherals disconnect.                |

## LED Feedback display keycodes

These [`&led_fb`](keycodes.md#general-keycodes) keycodes control whether the display is turned on or off:

| Keycode      | Action                                                                     |
| ------------ | -------------------------------------------------------------------------- |
| `LF_ON`      | Turn the display on.                                                       |
| `LF_OFF`     | Turn the display off.                                                      |
| `LF_TOG`     | Toggle the display on or off.                                              |
| `LF_BRI_INC` | Increase brightness, turns the display on, and shows a brightness preview. |
| `LF_BRI_DEC` | Decrease brightness, turns the display on, and shows a brightness preview. |

For details on all available keycodes, see the [keycodes documentation](keycodes.md).

## Keycodes affected while the display is off

These keycodes don't provide feedback when the display is turned off, unless [an override](#override-feedback-types) is provided in the [`CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES`](#configuration-options) option.

| Keycode                                                                                        | Override needed    |
| ---------------------------------------------------------------------------------------------- | ------------------ |
| [`LF_LAYER`](keycodes.md#general-keycodes)                                                     | `layer_feedback`   |
| [`&led_bat N`](keycodes.md#battery-feedback-keycodes)                                          | `battery_feedback` |
| ZMK [layer change keycodes](https://zmk.dev/docs/keymaps/behaviors#layer-navigation-behaviors) | `layer_change`     |

The [`LF_LAYER_TOG`](keycodes.md#general-keycodes) keycode toggles continuous layer indication but doesn't turn the display back on. Use [these keycodes](#led-feedback-display-keycodes) to turn the display back on.

For details on all available keycodes, see the [keycodes documentation](keycodes.md).
