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
| `CONFIG_ZMK_LED_FEEDBACK_WARNING_BLINKS`                 | `5`                             | Number of regular connection and battery warning blinks (1–1000).    |
| `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS`           | `-1`                            | Required device numbers to check for both warnings; `-1` means all.   |
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

Set a battery warning threshold in the dongle's zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file).
This example enables low-battery and no-reading warnings and triggers a low-battery warning when
a required, connected peripheral reports below 15%. Set it to `0` (default) to disable both
battery warnings.

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=15
```

The warning blinks red then yellow. At most four LEDs blink, regardless of strip length. It
doesn't display the device number; press the [battery display keys](#keycodes) to find which
device needs charging. Disconnected and optional peripherals are ignored.

Warnings appear once while their condition remains true and stop if it resolves. If a layer
change dismisses a low-battery warning, one red/yellow reminder blink follows the layer indication
if the condition still applies.

## No-reading warning

When battery warnings are enabled, a required, connected peripheral without a reading produces a
yellow-only warning. It uses up to four LEDs and does not identify the device. A reported level of
`0%` is treated as no reading. Optional and disconnected peripherals are ignored.

The dongle first checks for missing readings 30 seconds after boot, giving peripherals time to
report. After an inactivity reset it checks immediately. A peripheral connecting later may briefly
trigger this warning before its first reading arrives. The warning stops when the condition
resolves, such as when a reading arrives or the peripheral disconnects.

Connection, no-reading, and low-battery warnings use the same blink count and timing. For
configuration, priority, layer dismissal, and inactivity reset, see
[warning behavior](warnings.md).
