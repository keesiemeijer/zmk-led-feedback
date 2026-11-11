# Display on and off

The display starts enabled after every boot. Use the display keycodes to turn all LED feedback on
or off. There are no configuration options that start it off or allow feedback to bypass the off
state.

## Display controls

| Keycode      | Action                                                                |
| ------------ | --------------------------------------------------------------------- |
| `LF_ON`      | Turn the display on.                                                  |
| `LF_OFF`     | Turn the display off.                                                 |
| `LF_TOG`     | Toggle the display on or off.                                         |
| `LF_BRI_INC` | Increase brightness, turn the display on, and preview the brightness. |
| `LF_BRI_DEC` | Decrease brightness, turn the display on, and preview the brightness. |

See the [keycodes documentation](keycodes.md) for all available bindings.

## Display off during warnings

While off, the strip stays dark regardless of connection warnings, battery warnings, layer
feedback, battery requests, or brightness previews. Turning it off immediately stops a connection
warning. It also cancels an already-started battery warning; that sequence is considered consumed
and is never replayed.

A low report received while the display is off may remain pending if its warning has not started.
Turning the display back on before the 20-second boot window closes re-evaluates eligible warning
state. If the display remains off at the deadline, a pending battery warning is discarded, and
turning it on later does not start a missed boot warning. Ordinary layer and on-demand battery
feedback work again when the display is on.

Brightness keys always update brightness and turn the display on. If turning it on makes a boot
warning eligible, the warning appears instead of a brightness preview. Warnings take priority over
layer changes and on-demand battery requests; those requests are discarded and not replayed.

The former `CONFIG_ZMK_LED_FEEDBACK_DISPLAY_OFF` and
`CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES` options have been removed. Do not include them in your
configuration.
