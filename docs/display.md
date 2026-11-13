# Display on and off

The display starts enabled after every boot. Use the display keycodes to turn all LED feedback on
or off.

## Display controls

| Keycode      | Action                                                               |
| ------------ | -------------------------------------------------------------------- |
| `LF_ON`      | Turn the display on.                                                 |
| `LF_OFF`     | Turn the display off.                                                |
| `LF_TOG`     | Toggle the display on or off.                                        |
| `LF_BRI_INC` | Increase brightness, turns the display on to preview the brightness. |
| `LF_BRI_DEC` | Decrease brightness, turns the display on to preview the brightness. |

See the [keycodes documentation](keycodes.md) for all available bindings.

## Display off

When the display is off, layer feedback and battery status are suppressed. Consequently, `LF_LAYER`, `LF_LAYER_TOG`, `&led_bat N`, and standard ZMK [layer keycodes](https://zmk.dev/docs/keymaps/behaviors#layer-navigation-behaviors) do not update the display. See the [keycode documentation](keycodes.md) for details.

### Warnings while the display is off

While warnings are active you can turn the display off with the [display control keycodes](#display-controls). This hides [connection warnings](connection-feedback.md#connection-warnings) and cancels [low-battery warnings](battery-feedback.md#low-battery-warnings).

## Using brightness keys while the display is off

Brightness keys show a preview of the current brightness and will turn the display on to do so.
