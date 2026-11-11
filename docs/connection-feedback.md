# Connection feedback

Connection feedback warns when one or more required peripherals are disconnected. It is enabled
by default. See [warning behavior](warnings.md) for shared timing, dismissal, priority, and reset
options.

## Configuration

| Option                                                    | Default | Description                                                                               |
| --------------------------------------------------------- | ------- | ----------------------------------------------------------------------------------------- |
| `CONFIG_ZMK_LED_FEEDBACK_CONNECTION`                      | `y`     | Enable connection warnings for disconnected peripherals.                                  |
| `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` | `-1`    | Required device numbers for both warning types (`-1` means all, `0` none). |

## Connection warning

The warning shows the number of the highest-numbered missing [required peripheral](#required-peripherals). It blinks red LEDs equal to its [device number](peripheral-numbering.md#device-numbers-and-pairing-slots). For example, two blinking LEDs identify device 2.

If the strip has fewer LEDs than configured peripherals, it blinks all LEDs as a generic warning because it cannot reliably display each one. Only required peripherals are considered, optional peripherals are ignored.

After boot, a regular connection warning blinks the shared number of times, then stops even if a
peripheral remains disconnected. It only warns again after all required peripherals reconnect and
one disconnects, or an [inactivity reset](warnings.md#inactivity-reset) starts a new period.
At boot, a separate [continuous warning](warnings.md#boot-connection-warning) runs for up to
30 seconds. A disconnect may take time for Bluetooth to detect.

## Required peripherals

Required peripherals are checked by both connection and battery warnings. All configured peripheral
numbers are required by default, including slots that have not paired yet. Use
`CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` in the dongle's
[`.conf`](dongle-setup.md#dongle-configuration-file) to set the limit.

| Value | Result                                       |
| ----- | -------------------------------------------- |
| `-1`  | Default. All configured peripheral slots are required |
| `0`   | No peripherals are required                 |
| `1`   | Device number 1 is required                |
| `N`   | Device numbers 1 through N are required     |

In this example only device numbers 1 and 2 are required; higher-numbered devices do not trigger
connection or battery warnings. Device numbers normally follow pairing order but can be changed
with [`SLOT_ORDER`](peripheral-numbering.md#changing-the-pairing-order).

```ini
CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=2
```
