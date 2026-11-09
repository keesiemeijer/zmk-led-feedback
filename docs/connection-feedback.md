# Connection feedback

Connection feedback blinks red LEDs when one or more required peripherals are disconnected. It is
enabled by default.

## Configuration

| Option                                                    | Default | Description                                                                               |
| --------------------------------------------------------- | ------- | ----------------------------------------------------------------------------------------- |
| `CONFIG_ZMK_LED_FEEDBACK_CONNECTION`                      | `y`     | Enable connection warnings for disconnected peripherals.                                  |
| `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS`            | `-1`    | Number of required peripherals (`-1` means all). Higher-numbered peripherals are ignored. |

## Connection Warning

The missing connection warning shows the number of the highest-numbered missing [required peripheral](#required-peripherals). It blinks red LEDs equal to its [device number](peripheral-numbering.md#device-numbers-and-pairing-slots).  For example, two blinking LEDs identify device 2. 

If the strip has fewer LEDs than configured peripherals, it blinks all LEDs as a generic warning because it cannot reliably display each one. Only required peripherals are considered, optional peripherals are ignored.

## Required peripherals

Required peripherals trigger a [connection warning](#connection-warning). All configured peripheral slots are required by default. Use the `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` option to set specific peripherals as required. Use this option in your zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle.

This setting is available when either connection or battery feedback is enabled. It does not
filter normal low-battery warnings, which consider every peripheral. The optional
[startup diagnostic](startup-diagnostic.md) also uses it to determine which battery reports
are required.

| Value | Result                                               |
| ----- | ---------------------------------------------------- |
| `-1`  | Default. All configured peripheral slots are required |
| `0`   | No peripherals are required                           |
| `1`   | Device number 1 is required                           |
| `N`   | Device numbers 1 through N are required               |

Device numbers follow the configured [slot order](peripheral-numbering.md#changing-the-pairing-order).

In this example only the first two peripherals are required. If another device  paired after them and has a higher [pairing slot number](peripheral-numbering.md#device-numbers-and-pairing-slots) it will not trigger a connection warning if it’s not connected

```ini
CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=2
```
