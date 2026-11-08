# Battery feedback

// Todo: Remove option `CONFIG_ZMK_LED_FEEDBACK_BATTERY_DELAY_MS` from code and documentation

Battery feedback displays the battery level of a peripheral on request with keycodes, and can optionally
show a warning when a peripheral battery is low. It runs on the central. The dongle has no
battery of its own, so only peripheral batteries are displayed.

## Configuration

| Option                                                   | Default                         | Description                                                          |
| -------------------------------------------------------- | ------------------------------- | -------------------------------------------------------------------- |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY`                        | `n`                             | Enable the `led_bat` behavior and peripheral battery-level fetching. |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS`                 | `"00FF00,FFFF00,FF0000,0000FF"` | Battery colors in order: high, medium, low, no reading.              |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL`          | `0`                             | Warn below this battery percentage; `0` disables warnings.           |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_REPEAT_MINUTES` | `0`                             | Repeat interval while a battery remains low; `0` disables repeats.   |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS`         | `5`                             | Number of warning blinks (1–1000).                                   |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_DELAY_MS`               | `500`                           | Off time after a battery key press before the first level appears.   |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_DURATION_MS`            | `1000`                          | How long each device's battery level stays lit.                      |


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
report its battery.

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

Set a battery warning threshold in the zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle.  This example triggers a battery warning when any peripheral battery drops below 15%. Set it to `0` (default) to disable warnings.

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=15
```

The warning blinks red then yellow. At most four LEDs blink, regardless of strip length. The warnings don’t display the device number (as connection warnings do). Press the [battery display keys](#keycodes) for each device to find which one needs charging.

If a layer change interrupts a warning, the warning is dismissed. One red/yellow blink is shown after the layer indication clears if the battery is still low.

The shared CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS option controls the duration of each red or yellow warning color. Set an interval duration in the zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle.

Example setting the blink duration to 800 milliseconds.

```ini
CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=800
```

### Battery warning repeat
Warnings do not repeat by default. To repeat while any battery remains low, set an interval
in minutes in the zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle.

Example setting the battery warning interval to 5 minutes.

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_REPEAT_MINUTES=5
```

Repeats stop when no peripheral is below the threshold.

### Battery warning blinks
The default battery warning is 5 warning blinks long with a short pause between them. Set a different amount in the zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle.

Example setting the battery warnings to 10 blinks long:

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=10
```
