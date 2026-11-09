# Startup diagnostic specification

Status: proposed; not yet implemented.

## Purpose and scope

Provide an opt-in startup diagnostic for connection and battery feedback on the central. It
helps users check for missing connections, low batteries, and missing battery reports without
requiring the diagnostic to run continuously.

The diagnostic is intended for setups where every required peripheral can report its battery
when battery feedback is enabled. Users with mixed setups that cannot meet this expectation can
leave the diagnostic disabled and continue using the existing feedback behavior.

## Lifetime and configuration

- Enable the diagnostic with `CONFIG_ZMK_LED_FEEDBACK_STARTUP_DIAGNOSTIC`. It is disabled by
  default.
- When enabled, it runs for a fixed 20 seconds beginning at central boot. Connection and battery
  events do not restart or extend this window.
- During that window, the diagnostic temporarily overrides the normal warning policy. At the
  deadline it stops diagnostic warnings and hands control back to the existing configured
  warning behavior. It does not permanently disable warnings.
- After the deadline, connection warnings and battery warnings and repeats behave according
  to the user's normal settings. Warnings disabled by those settings remain disabled.
- When the diagnostic is disabled, existing runtime feedback and warning behavior is unchanged.
  The required-peripherals configuration option is renamed as described below.

This feature targets a USB-powered dongle; a separate wake-from-sleep diagnostic is not part of
the agreed behavior.

## Status and warning priority

Throughout the window, continuously re-evaluate the status of each enabled feedback type.
Show warnings without a startup grace period and choose the highest applicable diagnostic
warning. Red or yellow warnings may therefore appear briefly while peripherals connect or send
their first battery reports:

1. **Connection warning:** If connection feedback is enabled and a required peripheral is
   disconnected, show the existing connection warning. It takes precedence over battery
   diagnostics.
2. **Low-battery warning:** Otherwise, if battery feedback is enabled and any available
   peripheral battery report is below the warning threshold, show the existing generic
   red/yellow low-battery warning. Use the existing low-battery logic, which considers any
   reported peripheral, not just required peripherals.
3. **Missing-report warning:** Otherwise, if battery feedback is enabled and at least one
   required peripheral has no battery report, blink yellow using up to four LEDs. This is a
   generic warning and does not identify a peripheral.
4. **No diagnostic warning:** Otherwise, show no diagnostic warning.

If connection feedback is disabled, do not wait for connection feedback to resolve before
evaluating battery status. If battery feedback is disabled, do not show either battery
diagnostic.

Changes can move the diagnostic between these states in either direction. For example, if a
required peripheral disconnects after the battery diagnostic has begun, the connection warning
takes priority again. A known low battery takes priority over missing battery reports.

A healthy report from one peripheral does not hide a missing report from another required
peripheral. The existing battery keycodes can be used to investigate which peripheral has no
reading. A peripheral that never reports a battery may therefore cause a yellow warning until
the diagnostic ends.

### Required peripherals

Move and rename the existing connection-specific setting to the general setting
`CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS`. It is available when either connection or
battery feedback is enabled and retains the current values and device numbering:

- `-1` means all configured peripheral slots are required.
- `N` means devices numbered 1 through `N` are required, respecting the configured slot order.
- `0` means no peripherals are required, so the diagnostic does not show a missing-report warning.

Connection feedback uses this setting for disconnected-peripheral warnings. For battery feedback,
the setting only determines which reports the startup diagnostic expects; it does not filter
low-battery warnings during or after the diagnostic. A low report from any peripheral can
trigger those warnings.

Backward compatibility with `CONFIG_ZMK_LED_FEEDBACK_CONNECTION_REQUIRED_PERIPHERALS` is not
required. Replace the old name in code and documentation rather than supporting both names.

### Battery warning threshold

The diagnostic uses a fixed low-battery threshold of 15%, independent of the configurable normal
warning threshold. As with the existing warning logic, a reported battery is low only when its
level is below the threshold: 14% triggers a warning but 15% does not.

The diagnostic uses its own threshold during the startup window. Afterward, normal warnings use
the existing threshold; a normal threshold of `0` keeps those warnings disabled. The
missing-report warning is independent of either threshold.

The current implementation treats a battery level of `0` as no reading, including when a
peripheral has not reported or has disconnected. The diagnostic must account for this existing
representation.

### Warning timing

The highest-priority applicable warning continues throughout the diagnostic window while its
condition persists, except when interrupted by feedback or suppressed by display controls.
Status changes are reflected promptly rather than waiting for a pattern to finish.

- Connection warnings use the existing connection blink timing.
- Missing-report warnings blink yellow on and off, using
  `CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS` for each phase.
- Low-battery warnings continuously repeat the existing red, yellow, and one-second dark pause
  cycle. Each color uses `CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS`.

When the warning type changes, start the new pattern from its beginning: red for connection or
low-battery warnings, yellow for a missing-report warning. After an interruption, re-evaluate
status and restart the appropriate pattern from its beginning.

The normal battery-warning blink-count and repeat-interval settings do not limit diagnostic
warnings. They apply again after the diagnostic ends.

When no diagnostic warning is active, show the always-on layer if configured and visible.
During connection-warning blink gaps, show the always-on layer as in normal connection feedback.
Keep battery-warning gaps dark so their patterns remain distinct.

## Display

If connection or battery feedback is enabled, the diagnostic makes warnings visible during the
startup window even if the display was initially configured off. The temporary display-on state
is separate from the user's display setting. If no display-control keycode is pressed, the
original display setting is restored when the diagnostic ends.

While temporarily on, the display is effectively on for ordinary feedback as well as diagnostic
warnings. Battery requests, layer feedback, and other ordinary feedback do not need display-off
overrides until the user explicitly turns the display off.

Explicit display controls take precedence during the diagnostic:

| Keycode | Behavior during the diagnostic |
| --- | --- |
| `LF_OFF` | Turn the display off immediately and keep it off after the diagnostic. |
| `LF_ON` | Turn the display on and keep it on after the diagnostic. |
| `LF_TOG` | Toggle the currently effective display state and keep the result after the diagnostic. |
| `LF_BRI_INC`, `LF_BRI_DEC` | Change brightness and turn the display on, as usual; keep it on after the diagnostic. |

For example, if the display was configured off but the diagnostic temporarily turned it on,
`LF_TOG` turns it off. An explicit off suppresses diagnostic warnings for the rest of the window
unless the display is turned back on; status continues to be evaluated and the deadline does
not change. Other feedback types remain subject to their normal display-off overrides; for
example, `battery_feedback` can allow an explicit battery reading while the display is off.
Once that feedback finishes, the display returns to its off state and diagnostic warnings remain
suppressed unless the user turns the display on again.

After the deadline, normal display settings and display-off overrides apply to normal feedback.
The diagnostic's temporary visibility must not permanently change those settings.

## Feedback and layer interruptions

Explicit battery and layer feedback requests, layer changes, and brightness previews temporarily
interrupt diagnostic warnings so their usual feedback can be shown. Normal typing does not
interrupt warnings. These interruptions do not extend the 20-second deadline.

When the interrupting feedback finishes while the diagnostic is still active, re-evaluate the
current status and show the appropriate diagnostic warning, if any. In particular, a layer
change temporarily interrupts a diagnostic battery warning rather than dismissing it and
limiting it to the single reminder blink used by normal battery warning behavior.

After the diagnostic ends, normal configured behavior takes over, including the existing
single-blink battery warning reminder after a layer change.

## Deadline handoff

At the 20-second deadline, stop any diagnostic warning pattern and clear the temporary diagnostic
warning state. Stop all in-progress warnings and feedback displays immediately, including explicit
battery or layer displays. Retain the latest observed connection status and battery reports, and
start normal checks afresh using that data and the user's configured warning rules. Do not resume
diagnostic warnings after the deadline.

Pending or already-running diagnostic work must not write to the strip or reschedule diagnostic
warnings after normal feedback takes over.

Keep the user's latest display setting selected with a display-control keycode; if none was used,
restore the original boot setting. Restore the normal always-on layer indication when configured,
using the currently active layer and preserving any `LF_LAYER_TOG` changes made during the
diagnostic, subject to normal visibility and warning priority. Normal display-off overrides apply
immediately. If a retained battery report is below the normal threshold, start a normal
low-battery warning and schedule repeats as configured, subject to normal display-off and warning
rules. A normal threshold of `0` disables normal low-battery warnings. Missing battery reports do
not trigger a warning after handoff, though battery keycodes can still display an unknown reading.

For a normal low-battery warning at handoff, show it if the display is on or the
`battery_warning` off override is allowed. Otherwise suppress it, while still scheduling any
configured repeats. Turning the display on does not itself replay the suppressed warning; a
configured repeat may show it later if normal visibility rules then permit it.

## Acceptance scenarios

- With the diagnostic disabled, existing runtime feedback and warning behavior remains unchanged;
  configuration using the old required-peripherals option name must be updated.
- With both feedback types enabled, a missing required connection is shown before battery
  diagnostics. Once the connection is resolved, battery status is evaluated.
- A known low battery produces the low-battery warning even if another required battery report
  is missing. A low report from an optional peripheral also qualifies under the existing logic.
- At the fixed diagnostic threshold, a 14% report triggers a low-battery warning and a 15%
  report does not. A normal threshold of `0` does not prevent diagnostic low-battery warnings
  and disables them after the handoff.
- A healthy report from one peripheral and a missing report from another required peripheral
  produce the generic yellow warning. All required reports present and no low reported battery
  produce no diagnostic warning.
- With connection feedback disabled, battery feedback uses the general required-peripherals
  setting for missing reports, including its slot order. With zero required peripherals, no
  missing-report warning is shown; a low report can still trigger a low-battery warning.
- A new required disconnection during the window causes the connection warning to take priority
  again. No status change extends the deadline.
- Missing-report warnings blink continuously in yellow; low-battery warnings repeat the
  red/yellow/dark cycle without being limited by the normal blink-count or repeat settings.
- A change of warning type or the end of an interruption starts the appropriate pattern from
  its beginning rather than carrying over an earlier pattern's phase.
- An always-on layer is shown when there is no diagnostic warning and during connection-warning
  gaps, subject to visibility. Battery-warning gaps remain dark.
- A feedback request or layer change temporarily interrupts a diagnostic warning; once that
  feedback finishes, the current status determines which warning, if any, is shown. A layer
  change does not dismiss a diagnostic battery warning.
- When the display starts off, diagnostic warnings are visible during the window and the display
  returns off at the deadline if no display-control keycode is pressed.
- When the diagnostic has temporarily turned the display on, battery requests and layer feedback
  can be shown without display-off overrides.
- When the diagnostic has temporarily turned the display on, `LF_TOG` or `LF_OFF` turns it off
  immediately and suppresses diagnostic warnings until it is turned on again. The selected state
  persists after the deadline; other feedback remains subject to its normal display-off overrides
  both during and after the diagnostic.
- With the display explicitly off, an allowed battery feedback request can display its reading;
  after it finishes, diagnostic warnings remain suppressed until the display is turned on.
- `LF_ON` and brightness keys turn the display on during the diagnostic and leave it on after the
  deadline.
- At 20 seconds after central boot, diagnostic warnings stop and normal configured warning
  behavior takes over, whether that means continuing warnings or not.
- Connection status and battery reports observed during the diagnostic are retained at the
  deadline. Normal checks start afresh using that data, and normal warning settings and
  display-off overrides apply. The latest user-selected display state is retained.
- A retained low-battery report triggers a normal warning and configured repeats at handoff,
  subject to normal visibility and warning rules. A configured always-on layer is restored
  subject to the same normal rules, using the current layer and any `LF_LAYER_TOG` changes made
  during the diagnostic.
- At handoff, a low-battery warning is visible with the display off only if the
  `battery_warning` override permits it. Otherwise it is suppressed, but configured repeats
  remain scheduled; turning the display on does not itself replay the suppressed warning.
- There is no startup grace period; connection and missing-report warnings may appear briefly
  as peripherals connect and report.
- Missing battery reports do not trigger a warning after handoff, but remain available as
  unknown readings through battery keycodes.
- An explicit battery or layer display in progress at the deadline stops immediately. The
  diagnostic warning does not resume.
- If diagnostic blink work and the handoff become ready nearly simultaneously, no diagnostic
  update overwrites normal feedback or schedules another diagnostic warning after handoff.

See [connection feedback](connection-feedback.md), [battery feedback](battery-feedback.md),
[display-off behavior](display-off.md), and [keycodes](keycodes.md) for existing behavior.
