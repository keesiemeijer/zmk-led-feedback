# Connection feedback

Connection feedback is an optional, boot-only warning. When enabled, it blinks red LEDs while a
required peripheral is missing during the first 20 seconds after module initialization. It is
disabled by default.

## Configuration

| Option                                         | Default | Description                                                                                |
| ---------------------------------------------- | ------- | ------------------------------------------------------------------------------------------ |
| `CONFIG_ZMK_LED_FEEDBACK_CONNECTION`           | `n`     | Enable connection warnings during the boot window.                                         |
| `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` | `-1`    | Shared warning filter: `-1` means all, `0` none, or `1..N` mapped device numbers required. |

The warning starts immediately for missing required devices. It stops when they reconnect. If a
device is still missing at the 20-second deadline, the current warning ends there if it has shown
at least two full red blinks; otherwise it finishes the two-blink minimum. During this brief
continuation, only devices already missing at the deadline are shown. Reconnecting one removes it
from the warning, and a disconnection processed at or after the deadline cannot add a device.
Turning the display off stops the warning immediately.

Connection warnings take priority over battery warnings and ordinary layer or on-demand feedback.
A pending battery warning runs after the connection warning ends. Turning the display off cancels
all visible warning output; see [display-off behavior](display-off.md).

## Connection Warning

The missing connection warning shows the number of the highest-numbered missing [required peripheral](#required-peripherals). It blinks red LEDs equal to its mapped [device number](peripheral-numbering.md#device-numbers-and-pairing-slots). For example, two blinking LEDs identify device 2.

If the strip has fewer LEDs than configured peripherals, it blinks all LEDs as a generic warning because it cannot reliably display each one. Only required peripherals are considered, optional peripherals are ignored.

## Required peripherals

Required peripherals trigger both connection and battery warnings. All configured peripheral slots are required by default. Use the shared `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` option to set the number of mapped device numbers that are required. Use this option in your zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file) file for the dongle. Optional devices are still available for on-demand battery display.

| Value | Result                                         |
| ----- | ---------------------------------------------- |
| `-1`  | Default. All configured peripheral slots       |
| `0`   | No peripherals are required for either warning |
| `1`   | Device number 1 is required                    |
| `N`   | Device numbers `1..N` are required             |

The required count cannot exceed the configured number of peripheral slots. The
`CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER` mapping determines device numbers; see
[peripheral numbering](peripheral-numbering.md#changing-the-pairing-order) for its format.

In this example, only device numbers 1 and 2 are required. Higher-numbered devices are optional
and do not trigger or block either warning:

```ini
CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=2
```
