# Warning behavior

The dongle can warn about a missing [connection](connection-feedback.md#connection-warning), a
[low battery](battery-feedback.md#low-battery-warnings), or a connected peripheral with
[no battery reading](battery-feedback.md#no-reading-warning). Connection feedback is on by default;
battery warnings are disabled by default.

## Shared configuration

Set these options in the dongle's zmk-config [`.conf`](dongle-setup.md#dongle-configuration-file):

| Option | Default | Description |
| ------ | ------- | ----------- |
| `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` | `-1` | Required device numbers, `-1` for all or `0` for none; range `-1`–`16`. Both connection and battery warnings ignore devices above this number. |
| `CONFIG_ZMK_LED_FEEDBACK_WARNING_BLINKS` | `5` | Number of blinks for each regular warning; range `1`–`1000`. |
| `CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS` | `500` | Duration of each of the two steps in a blink, in milliseconds. |
| `CONFIG_ZMK_LED_FEEDBACK_WARNING_INACTIVITY_MINUTES` | `0` | Start a new warning period on a key press after inactivity: `0` disables it, otherwise `5`–`1440` minutes. |

Both warning types use the same blink count and interval, with a fixed one-second pause between
blinks. A regular warning lasts approximately `blinks × (2 × interval + 1 second)`, or about
10 seconds with the defaults; there is no final one-second pause.

For example:

```ini
CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=2
CONFIG_ZMK_LED_FEEDBACK_WARNING_BLINKS=3
CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=500
```

See [required peripherals](connection-feedback.md#required-peripherals) for how device numbers
are assigned.

## When warnings appear

Each condition warns once while it remains true. A warning stops if its condition resolves; it
can appear again when the condition becomes true again. There is no periodic repeat. Disconnecting
a peripheral can take time to detect, so a warning stops or starts when the dongle detects the
change, not necessarily when the peripheral is powered off.

Only one warning runs at a time. When multiple warnings are due, the order is connection, no
reading, then low battery. A running warning is not interrupted by a higher-priority warning:
the latter waits. If its condition resolves while waiting, it is dropped. A large blink count can
therefore delay other warnings for a long time: 1000 blinks takes about 33 minutes at the defaults.

A visible layer indication dismisses a running regular warning. Once the indication clears, a
single reminder blink appears if the condition still applies; the full warning is not repeated.
The reminder uses the warning's own appearance. A layer change after the warning ends does not
give a reminder. A requested battery display also takes over the strip; an interrupted warning
can start again when the display finishes.

## Boot connection warning

At boot, if a required peripheral is missing and connection feedback is enabled, the dongle
blinks red continuously, without the regular one-second pauses. It stops when all required
peripherals connect or after 30 seconds, whichever comes first. The regular blink count does not
apply. If a peripheral is still missing at 30 seconds, it counts as warned; a regular warning
requires all required peripherals to connect and a later disconnect.

A layer indication interrupts the boot warning, which resumes without a reminder when the
indication clears. The 30-second window continues during the indication. Battery warnings wait
while the boot warning is active.

## Inactivity reset

With `CONFIG_ZMK_LED_FEEDBACK_WARNING_INACTIVITY_MINUTES` set to `5`–`1440`, the first key press
after that many minutes without a key press starts a new warning period. The interval is counted
from boot until the first press, and from the last press thereafter. Nothing happens just because
the interval expires. This is independent of ZMK idle and sleep settings.

The reset stops any running warning, discards waiting warnings, and checks conditions again. A
missing required peripheral starts a new continuous connection warning for up to 30 seconds.
Battery conditions are checked as new events; no-reading checks do not wait another 30 seconds.
If the triggering key changes the layer, its indication can appear before the new warning or
interrupt it under the normal rules. In the latter case a regular warning may be shortened to a
single reminder blink.

The option defaults to `0` (off). It has no effect when connection feedback is off and the
battery warning level is `0`, because there are no warnings.

## Display off

When the display is off, warnings are only visible if their type is allowed by
[`CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES`](display-off.md#override-feedback-types). Warning
conditions and periods still run; a suppressed regular warning counts as shown and is not
automatically replayed when the display is turned back on.

## Renamed and removed options

- Replace `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS` with
  `CONFIG_ZMK_LED_FEEDBACK_WARNING_BLINKS`.
- Replace `CONFIG_ZMK_LED_FEEDBACK_CONNECTION_REQUIRED_PERIPHERALS` with
  `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS`.
- Remove `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_REPEAT_MINUTES`; there is no repeat option.
  The optional inactivity reset starts a new period only when a key is pressed.
