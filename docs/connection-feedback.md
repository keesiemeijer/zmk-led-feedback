# Connection feedback

Connection feedback is an optional, boot-only warning. When enabled, it blinks red LEDs while a
required peripheral is missing during the first 20 seconds after module initialization. It is
disabled by default.

## Configuration

| Option                                         | Default | Description                                                                                |
| ---------------------------------------------- | ------- | ------------------------------------------------------------------------------------------ |
| `CONFIG_ZMK_LED_FEEDBACK_CONNECTION`           | `n`     | Enable connection warnings during the boot window.                                         |
| `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` | `-1`    | Shared warning filter: `-1` means all, `0` none, or `1..N` mapped device numbers required. |

For a missing required device, a connection warning starts when a complete red phase fits before
the 20-second deadline. Immediately before rendering each red phase, the module checks whether the
configured blink interval fits in the remaining time. A phase ending exactly at the deadline
counts as complete; one that would overrun is suppressed rather than shortened. A queued phase
that is suppressed does not interrupt an active battery warning. Connection-warning output never
continues after boot; at the logical deadline the module closes warning state and clears the strip
when deadline handling runs, subject to normal scheduling latency. Turning the display off stops
the warning immediately.

Once rendered, connection warnings take priority over battery warnings and ordinary layer or
on-demand feedback. A pending battery warning runs after rendered connection output ends or is
suppressed at the deadline. Turning the display off cancels all visible warning output; see
[display-off behavior](display-off.md).

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
