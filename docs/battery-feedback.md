# Battery feedback

Battery feedback displays the battery level of a peripheral on request with keycodes, and can optionally
show a warning when a peripheral battery is low. It runs on the central. The dongle has no
battery of its own, so only peripheral batteries are displayed.

## Configuration

| Option                                           | Default                         | Description                                                        |
| ------------------------------------------------ | ------------------------------- | ------------------------------------------------------------------ |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY`                | `n`                             | Enable `led_bat` and peripheral battery-level fetching.            |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS`         | `"00FF00,FFFF00,FF0000,0000FF"` | Battery colors in order: high, medium, low, no reading.            |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL`  | `0`                             | Warning opt-in: `0` disables; `2-100` warns below the threshold.   |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS` | `5`                             | Battery warning cycles (`1-10`), completed at most once per boot.  |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_DELAY_MS`       | `500`                           | Off time after a battery key press before the first level appears. |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_DURATION_MS`    | `1000`                          | How long each device's battery level stays lit.                    |

### Keycodes

Use `&led_bat <device number>` to display a peripheral's battery:

| Binding      | Action                                                 |
| ------------ | ------------------------------------------------------ |
| `&led_bat 0` | Show all peripheral battery levels, one after another. |
| `&led_bat 1` | Show the battery level of peripheral device 1.         |
| `&led_bat N` | Show the battery level for device `N`.                 |

Device numbers follow [pairing order](peripheral-numbering.md#device-numbers-and-pairing-slots) unless changed with [`CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER`](peripheral-numbering.md#changing-the-pairing-order).

For more information about battery keycodes and behavior see [the keycodes documentation](keycodes.md#led-feedback-battery-behavior).

## Enabling battery feedback

Enable battery feedback in the zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle:

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
```

This also enables [`CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING`](https://zmk.dev/docs/config/battery#peripheral-battery-monitoring), so the central receives battery levels from its peripherals. Enable battery reporting on each peripheral with
[`CONFIG_ZMK_BATTERY_REPORTING`](https://zmk.dev/docs/config/battery#kconfig) (enabled by default on boards with a battery-voltage reading, such as the nice!nano).

Use the [battery feedback keys](keycodes.md#battery-feedback-keycodes) to show a peripheral's battery level.

The default duration of battery feedback is 1 second. Change the duration in the zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle.

Example to set battery display duration to 2 seconds after pressing a battery feedback key.

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY_DURATION_MS=2000
```

## Battery display colors and scale

The default colors are green above 60%, yellow from 20% through 60%, red below 20%, and blue if there is
no reading. No reading means the peripheral is disconnected, has not reported yet, or does not
report its battery. A zero reading is also treated as unavailable and displays one no-reading LED.
ZMK v0.3 does not distinguish a real 0% report from the zero value sent when a peripheral disconnects,
so a genuine 0% reading cannot trigger a warning.

Change the four colors with a comma-separated list of `RRGGBB` hex values in high, medium, low,
no-reading order. Each value must be exactly six hex digits with no `#`; invalid values fail the
build. Colors may be the same, but use a distinct no-reading color so it cannot be confused with
a low battery:

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS="0000FF,FF00FF,FF8000,808080"
```

See [Battery display scale](battery-display-scale.md) for percentage, color, and LED-count tables
for strips with 1 to 10 LEDs.

## Low-battery warnings

Enable battery feedback, then set a threshold in the dongle's zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file). A nonzero report below the threshold from a required peripheral during the first 20 seconds after module initialization latches one aggregate warning for that boot. Set the threshold to `0` (default) for on-demand battery display without warnings. Values `2-100` are accepted; `1` is invalid.

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=15
```

The warning blinks red then yellow, using at most four LEDs. It does not identify a device; press the [battery display keys](#keycodes) to check individual peripherals. The low report remains latched if a later report is healthy or unavailable. Missing reports and zero readings do not trigger a warning.

At most one complete battery warning runs per boot. Connection warnings take priority. If a connection warning interrupts a partial battery sequence, the full sequence restarts when the connection warning ends, possibly after the 20-second window. A warning that continues past the deadline still completes.

Layer changes, on-demand battery requests, and brightness previews do not replace or resume after a warning. Turning the display off suppresses all output and cancels an already-started battery warning without replay. See [display-off behavior](display-off.md) for pending warnings and re-enabling the display.

`CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS` controls each red/yellow color phase and connection red/dark phase. The default is 500 ms; accepted values are `100-1000` ms. Set it in the dongle's zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file).

Example setting the blink duration to 800 milliseconds.

```ini
CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=800
```

### Battery warning blinks

The default battery warning has five red/yellow cycles, with a one-second dark pause between cycles and no trailing pause. It takes nine seconds at the default 500 ms phase interval. Set `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS` to `1-10` in the dongle's zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file).

For example, set the warning to 10 cycles:

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=10
```
