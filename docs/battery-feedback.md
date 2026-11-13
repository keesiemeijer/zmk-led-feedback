# Battery feedback

Battery feedback displays the battery level of a peripheral on request with keycodes, and can optionally
show a warning when a peripheral battery is low. It runs on the central. The dongle has no
battery of its own, so only peripheral batteries are displayed.

## Configuration

| Option                                           | Default                         | Description                                                         |
| ------------------------------------------------ | ------------------------------- | ------------------------------------------------------------------- |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY`                | `n`                             | Enable battery feedback.                                            |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS`         | `"00FF00,FFFF00,FF0000,0000FF"` | Battery colors in order: high, medium, low, no reading.             |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL`  | `0`                             | `0` No low battery warnings; `2-100` warnings below this threshold. |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS` | `5`                             | Battery warning blinks (`1-10`)                                     |
| `CONFIG_ZMK_LED_FEEDBACK_BATTERY_DURATION_MS`    | `1000`                          | How long battery dislpay stays lit (after a layer change).          |

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

This also enables the ZMK option [`CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING`](https://zmk.dev/docs/config/battery#peripheral-battery-monitoring), so the central receives battery levels from its peripherals. Enable battery reporting on each peripheral with the ZMK option
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

Low battery warnings are only displayed during the first 20 seconds after booting the dongle. See [battery behavior](../README.md#warning-behavior)

Set a battery warning threshold in the zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle. Use `0` for no warnings or a value between `2` and `100` for the warning threshold.

This example triggers a battery warning when any peripheral battery drops below 15%. Set it to `0` (default) to disable warnings.

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=15
```

The warning blinks red then yellow. At most four LEDs blink, regardless of strip length. The warnings don’t display the device number (as connection warnings do). Press the [battery feedback keys](#keycodes) for each device to find which one needs charging.

If a [connection warning](connection-feedback.md#connection-warning) interrupts a battery warning (because it has priority), the full battery warning restarts when the connection warning ends

While a warning is active, layer feedback and on-demand battery displays are suppressed. Turning the display off cancels any battery-warning. See [display-off behavior](display.md#display-off).

### Required peripherals

Only required peripherals are used for battery (and connection) warnings. All paired peripherals are required by default. Use the shared [`CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS`](peripheral-numbering.md#required-peripherals) option to set the number of specific peripherals as required.

### Battery warning blinks

The default battery warning is 5 warning blinks long with a short pause between them. Set a different amount in the zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle.

Example setting the battery warnings to 10 blinks long:

For example, set the warning to 10 cycles:

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=10
```

The general `CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS` option controls the duration of each red or yellow warning color. Set an interval duration in the zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle. Accepted values are between `100` and `1000` miliseconds.

Example setting the blink duration to 800 milliseconds.

```ini
CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=800
```
