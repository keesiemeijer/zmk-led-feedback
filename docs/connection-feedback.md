# Connection feedback

Connection warnings are only displayed during the first 20 seconds after booting the dongle. See [warning behavior](../README.md#warning-behavior). The warning blinks red LEDs while a required peripheral is missing. Connection warnings are disabled by default.

## Configuration

| Option                                         | Default | Description                                                                                |
| ---------------------------------------------- | ------- | ------------------------------------------------------------------------------------------ |
| `CONFIG_ZMK_LED_FEEDBACK_CONNECTION`           | `n`     | Enable connection warnings during the boot window.                                         |
| `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` | `-1`    | Shared warning filter: `-1` means all, `0` none, or `1..N` mapped device numbers required. |

A connection warning takes priority over a low battery warning. Layer and battery feedback display is suppressed during warnings. Turning the display off hides the connection warning output. See [display-off behavior](display.md#display-off).

## Connection Warnings

The missing connection warning shows the number of the highest-numbered missing [required peripheral](#required-peripherals). It blinks red LEDs equal to its mapped [device number](peripheral-numbering.md#device-numbers-and-pairing-slots). For example, two blinking LEDs identify device 2.

If the strip has fewer LEDs than configured peripherals, it blinks all LEDs as a generic warning because it cannot reliably display each one. Only required peripherals are considered, optional peripherals are ignored.

### Required peripherals

Only required peripherals are used for connection (and battery) warnings. All paired peripherals are required by default. Use the shared [`CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS`](peripheral-numbering.md#required-peripherals) option to set the number of specific peripherals as required.
