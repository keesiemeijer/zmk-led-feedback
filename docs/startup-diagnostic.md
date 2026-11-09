# Startup diagnostic

The optional startup diagnostic checks peripheral connections and battery reports for the first
20 seconds after the central boots. It is disabled by default. When it ends, normal configured
feedback and warning behavior takes over.

## Configuration

Enable the diagnostic in the dongle's [`.conf`](dongle-setup.md#dongle-configuration-file) file:

```ini
CONFIG_ZMK_LED_FEEDBACK_STARTUP_DIAGNOSTIC=y
```

At least one of connection or battery feedback must be enabled. Connection feedback is enabled
by default; enable battery feedback separately to check battery reports:

```ini
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
```

Battery feedback fetches peripheral battery levels; each required peripheral needs to support
and enable battery reporting. If a required peripheral never reports a battery, the diagnostic
can show a missing-report warning until the window ends. Mixed setups that cannot meet this
expectation can leave the diagnostic disabled.

[`CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS`](connection-feedback.md#required-peripherals)
determines which connections and battery reports are required. It defaults to all configured
peripheral slots; `0` means none are required. Device numbers follow the configured
[slot order](peripheral-numbering.md#changing-the-pairing-order). Low-battery warnings still
consider reports from **any** peripheral, including optional ones.

## Diagnostic warnings

During the window, the diagnostic continually checks the enabled feedback types and shows the
highest-priority condition:

1. **Red blinking:** a required peripheral is disconnected, when connection feedback is enabled.
   The [connection warning](connection-feedback.md#connection-warning) identifies the highest
   numbered missing peripheral when the strip is long enough.
2. **Red and yellow blinking:** any reported peripheral battery is below **15%**, when battery
   feedback is enabled. Up to four LEDs blink. This fixed threshold is independent of the normal
   warning threshold: 14% triggers it, while 15% does not.
3. **Yellow blinking:** at least one required peripheral has not supplied a battery report, when
   battery feedback is enabled. Up to four LEDs blink; the pattern does not identify a device.
4. **No diagnostic warning:** no applicable condition remains.

A known low battery takes priority over a missing battery report. If connection feedback is
disabled, battery checks do not wait for connections to resolve. Warnings can appear immediately
at boot while peripherals connect and send their first reports; there is no grace period.
Battery reports of `0` are treated as no reading, so an actual 0% report cannot be distinguished
from a missing reading. Use the
[battery keycodes](battery-feedback.md#keycodes) to investigate individual readings.

Patterns use `CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS`. Low-battery blinks repeat with a
one-second dark pause; the normal battery warning blink-count and repeat settings do not limit
the diagnostic. Connection warnings show an always-on layer during their gaps when applicable;
battery warning gaps stay dark.

## Display and keycodes

The diagnostic temporarily turns the display on, even if it is configured to start off.
Ordinary feedback, including layer changes and battery requests, is also visible. Such feedback
temporarily interrupts a diagnostic warning; when it finishes, the current status is checked
again and the appropriate warning restarts.

`LF_OFF` turns the display off and suppresses diagnostic warnings. `LF_TOG` toggles the
currently effective state, including the temporary on-state. `LF_ON` and brightness keys turn
the display on. Explicit display-control choices remain in effect after the diagnostic; without
one, the original display setting is restored.

While explicitly off, other feedback can still use its normal
[display-off overrides](display-off.md#override-feedback-types). For example, an allowed battery
request can show a reading, but diagnostic warnings remain suppressed afterward until the
display is turned on again.

## After 20 seconds

At the deadline, diagnostic warnings and in-progress feedback stop. The central retains the
latest connection and battery data and applies normal warning and display settings. A low
battery can trigger a normal warning and configured repeats; if the display is off, normal
warning overrides determine whether it is visible. A warning suppressed while off is not
replayed just by turning the display on, although configured repeats still apply. Missing
reports no longer trigger an automatic warning, but battery keycodes can still show that a
reading is unavailable.

The current layer and any changes made with display, brightness, and layer-toggle keys are
preserved. A configured always-on layer is restored subject to normal display and warning rules.
