# Spec: Unified warnings, event only (no repeat options)

Status: draft. Supersedes [spec-unified-warnings.md](spec-unified-warnings.md). Backwards
compatibility is not a concern, so options may be renamed or removed.

Difference with the superseded spec: there are no repeat options. A warning shows once for each
"event". A long time without key activity followed by a key press starts a new check period,
like a fresh boot (see [Inactivity reset](#inactivity-reset)).

## Goal

Connection and battery warnings behave the same way. They share their timing and peripheral
options, show once per event, and can be dismissed with a layer change.

## Current behavior (before this change)

| Aspect               | Connection warning                     | Battery warning                                                                   |
| -------------------- | -------------------------------------- | --------------------------------------------------------------------------------- |
| Length               | Blinks until the peripheral reconnects | `BATTERY_WARNING_BLINKS` (default 5)                                              |
| Blink duration       | `BLINK_INTERVAL_MS` (per color)        | `BLINK_INTERVAL_MS` (per color), 1 second pause between blinks                    |
| Peripheral selection | `CONNECTION_REQUIRED_PERIPHERALS`      | Any peripheral (required peripherals are not used)                                |
| Repeat               | None (continuous)                      | `BATTERY_WARNING_REPEAT_MINUTES` (default 0 = don't repeat)                       |
| Layer change         | Not defined                            | Dismisses the warning, one blink is shown after the layer indication if still low |

## Requirements

1. **Same length.** Both warnings use the same number of blinks and the same blink duration, so
   both warnings last equally long.
2. **Connection warnings stop.** A connection warning ends after the configured number of blinks,
   even if the peripheral is still disconnected.
3. **Shared required peripherals.** One option decides which peripherals are checked for both
   connection and battery level. Peripherals above the limit are ignored by both warnings.
   Disconnected peripherals are ignored by the battery warnings (the connection warning covers
   them).
4. **Once per event.** Every warning (connection, low battery, no reading) shows once per
   "event" (see [Warning lifecycle](#warning-lifecycle)). There are no repeat options. A long
   period without key activity starts a new check (see [Inactivity reset](#inactivity-reset)).
   The boot warning is not a regular warning (see [Boot warning](#boot-warning-connection-only)).
5. **Dismiss with a layer change.** A layer change dismisses an active warning of either type.
   One blink is shown after the layer indication clears if the condition still applies. Exception:
   the connection boot warning resumes blinking continuously instead (see
   [Boot warning](#boot-warning-connection-only)). Only connection warnings have a boot phase.

## Proposed options

### Shared (new / renamed)

| Option                                  | Default | Replaces                          | Notes                                  |
| --------------------------------------- | ------- | --------------------------------- | -------------------------------------- |
| `ZMK_LED_FEEDBACK_WARNING_BLINKS`       | `5`     | `BATTERY_WARNING_BLINKS`          | Range 1-1000. Used by both warnings.   |
| `ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS`    | `500`   | (existing, unchanged)             | Duration of each warning color.        |
| `ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` | `-1`    | `CONNECTION_REQUIRED_PERIPHERALS` | Range -1-16, `-1` = all. Used by both. |

### Per type

| Option                                        | Default | Notes                                                                        |
| --------------------------------------------- | ------- | ---------------------------------------------------------------------------- |
| `ZMK_LED_FEEDBACK_CONNECTION`                 | `y`     | Existing bool, unchanged. Enables connection warnings.                       |
| `ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL`      | `0`     | Existing (0 = battery warnings off).                                         |
| `ZMK_LED_FEEDBACK_WARNING_INACTIVITY_MINUTES` | `0`     | New. `0` = off, otherwise 5-1440. See [Inactivity reset](#inactivity-reset). |

The pause between blinks is fixed at 1 second for both warnings (not an option). Total length of
a warning is roughly blinks x (2 x `BLINK_INTERVAL_MS` + 1 s): about 10 seconds with the defaults.
Mention this in the docs.

Removed: `ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS`,
`ZMK_LED_FEEDBACK_CONNECTION_REQUIRED_PERIPHERALS` and
`ZMK_LED_FEEDBACK_BATTERY_WARNING_REPEAT_MINUTES`. No repeat option is added for connection.

`ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` must be available when either connection or battery
feedback is enabled (`depends on ZMK_SPLIT`).

## Warning lifecycle

For each warning type, the condition is:

- Connection: `ZMK_LED_FEEDBACK_CONNECTION` is enabled and at least one required peripheral is not
  connected.
- Battery, low: `BATTERY_WARNING_LEVEL` is above 0 and at least one required, connected peripheral
  is below it. Disconnected peripherals are ignored (the connection warning covers them and their
  last level is stale). A low battery warning stops immediately when that peripheral disconnects,
  and a new warning follows after it reconnects and reports a level below the threshold.
- Battery, no reading: `BATTERY_WARNING_LEVEL` is above 0 and at least one required, connected
  peripheral has not reported a battery level (see [No reading warning](#no-reading-warning)).

A warning "event" is the condition going from false to true. A peripheral that keeps connecting and
disconnecting gives a new warning each time all required peripherals were connected in between
(intended, no debounce).

Flow:

1. The condition becomes true: show the warning (N blinks).
2. While the condition stays true after the warning ended: nothing more is shown. There is no
   repeat, until a new check period starts (see [Inactivity reset](#inactivity-reset)).
3. The condition becomes false: the warning stops immediately, even in the middle of the blinks,
   and the state resets. The next time it becomes true it warns again.
4. Layer change while the warning is active: dismiss it (stop blinking, show the layer indication).
   After the layer indication clears, show one blink if the condition is still true. That blink
   is only a reminder: it is not a new warning. A dismissed warning is not shown again in full.
   The reminder blink looks the same as the warning's own blink: the connection warning shows the
   device number, low battery blinks red then yellow, no reading blinks yellow.
5. Layer change after a warning has already finished its blinks: no reminder blink. The reminder
   only follows an interrupted (active) warning. Decided: a missed warning gets no second chance
   (no reminder after it has finished).

### Boot warning (connection only)

Peripherals need time to connect after boot, so the connection warning starts with a boot phase:

1. At boot the connection warning starts immediately if a required peripheral is not connected.
   No delay and no option. Only runs when `ZMK_LED_FEEDBACK_CONNECTION` is enabled.
2. The boot warning is not a regular warning. It doesn't use up the "once per event" warning and
   the normal blink count doesn't apply. It blinks continuously (no pause, as today, because it
   is a different kind of warning) until every required peripheral is connected or 30 seconds
   have passed (fixed, not an option). Everyone who enables connection feedback and has missing
   peripherals at boot gets this warning.
3. All required peripherals connected within 30 seconds: the boot warning stops and the lifecycle
   above starts with a clean state. A later disconnect gives the regular warning (once).
4. 30 seconds passed with a required peripheral still missing: the boot warning stops and the
   condition counts as a warning that was shown. Nothing more is shown until the condition goes
   false and true again. Its state resets like any other warning's state: when all required
   peripherals are connected at the same time. A later drop is then a new event.
5. A layer change during the boot warning interrupts it (the layer indication has priority). After
   the layer indication clears it resumes blinking continuously for the rest of the 30 seconds,
   and can be interrupted again. There is no reminder blink. The 30 seconds keep counting during
   the layer indication.

## Inactivity reset

Without repeats a missed warning is gone. The inactivity reset gives a new chance at the moment
that matters: when the user comes back to the keyboard.

- **Own timer.** The module tracks the time of the last key activity itself. It doesn't use ZMK's
  idle or sleep state: idle uses `ZMK_IDLE_TIMEOUT` (30 seconds by default, shared with other
  features) and sleep never happens on USB power, so a USB powered dongle wouldn't work with it.
- **Activity** is a key position event (the same events ZMK counts as activity). Key presses on
  peripherals reach the central as position events (raised in ZMK's split central code), so a
  dongle sees all typing.
  Any key press counts, including display-off and battery keys.
- **Trigger.** When a key press happens after at least `WARNING_INACTIVITY_MINUTES` without
  activity, a new check period starts. The timer expiring alone doesn't start it, so nothing
  blinks while nobody is there. The inactivity time is counted from boot, so the time since boot
  counts as inactivity when nobody has typed yet. The boot warning already covered that moment, and
  a reset right after it is harmless.
- **What the new period does.** It behaves like a boot, with one difference for no reading:
    1. The state of all warnings is reset (all "already warned" flags are cleared).
    2. If a required peripheral is missing, the continuous
       [boot warning](#boot-warning-connection-only) runs for up to 30 seconds. It stops as soon
       as all required peripherals are connected (a sleeping peripheral wakes up when typing).
    3. Then the other conditions are evaluated as new events, in the normal priority order and
       without preemption: a low battery gives one low battery warning, a connected peripheral
       without a reading gives one no reading warning.
    4. The no reading condition is evaluated right away, not after 30 seconds. The delay at boot
       only exists because readings arrive after connecting, and after a long inactivity the
       peripherals have been connected for a long time.
- **A layer change caused by the triggering key** follows the normal layer-change rules. Its
  indication can appear before the new warning, or it can interrupt the warning after it starts.
  If it interrupts a regular warning, one reminder blink follows; if it interrupts the continuous
  connection warning, that warning resumes for the rest of its 30-second window. This applies to
  immediate layer keys and delayed activations such as hold-taps. There is no guarantee which
  order occurs; a full regular warning is not guaranteed if it is interrupted.
- **Option.** `ZMK_LED_FEEDBACK_WARNING_INACTIVITY_MINUTES`, default `0` (off), range 0-1440.
  Values 1 to 4 fail the build, so only `0` or 5-1440
  are valid. A very small value would make short breaks look like being away.
  When it is `0` there is no reset and the behavior is the same as without this section.
- A warning that is running when the reset happens is stopped, then the new period starts. A
  warning that is waiting behind it is dropped. Every condition that is still true is evaluated
  again in the new period. Example: a low battery warning is running and the connection warning
  waits. The reset stops the low battery warning and drops the waiting one, the boot warning
  runs for up to 30 seconds (it stops early when B connects), then the low battery shows as a
  new warning.
- Only runs when connection or battery feedback is enabled (`depends on ZMK_SPLIT`).
- With connection feedback off and `BATTERY_WARNING_LEVEL` at 0 there are no warnings, so the
  option does nothing. Mention this in the docs.

Example with the option set to 60: B dropped at 2 pm and the user didn't see the warning. The
user leaves at 3 pm and comes back at 9 am. The first key press at 9 am starts a new period:
B is still missing, so the boot warning blinks for up to 30 seconds. If B connects within
those 30 seconds, it stops.

## Warning priority

The warning types keep independent state, but never show at the same time:

- Priority: connection warning, then no reading warning, then low battery warning.
- A warning that is due waits and starts when the higher priority warning ends (it finished, was
  dismissed, or its condition resolved). It still shows if its condition is true at that point.
- A warning only waits for higher priority warnings that are active or due. A warning that is not
  due yet (for example the no reading warning in the first 30 seconds) blocks nothing. A running
  warning is never interrupted by a higher priority warning that becomes due later, that one waits.
  No preemption keeps the implementation simple: one active warning at a time, and when it ends
  the highest priority due warning starts.
  Accepted trade-off: with a very high `WARNING_BLINKS` (range up to 1000, about 33 minutes with
  the defaults) a running warning can delay a higher priority one for a long time. The docs
  should mention this.
- This includes the boot warning: battery warnings wait until the boot warning ends.

## No reading warning

A battery warning variant for when `BATTERY_WARNING_LEVEL` is above 0 and one or more required
peripherals are connected but don't report a battery level. A battery that can't be read can't be
confirmed healthy.

- Blinks yellow only (the low battery warning blinks red then yellow). It is part of the battery
  warning, not a third warning type: same `WARNING_BLINKS`, `BLINK_INTERVAL_MS`, fixed pause,
  layer change dismissal, stop on resolve and event definition. No new options.
- Disconnected peripherals are ignored, the connection warning covers them.
- Shows immediately when it is due, and only when no connection warning is active (or after it has
  ended). It lasts for the `WARNING_BLINKS` period.
- Readings arrive some time after a peripheral connects, so at boot the condition is only
  evaluated once 30 seconds have passed since boot (the same fixed window as the boot warning).
  After an [inactivity reset](#inactivity-reset) it is evaluated right away, because the
  peripherals have been connected for a while and have already reported. A peripheral that
  connects later may give a short warning until its first reading arrives (accepted).
- When a reading arrives or the peripheral disconnects, the condition is false and the warning
  stops immediately.
- If a peripheral has no reading and another is low, the no reading warning shows first. The low
  battery warning follows when it ends, if still low. Exception: the no reading warning isn't due
  in the first 30 seconds, so a low battery warning isn't delayed by it.
- Example: both peripherals connect 5 seconds after boot, A is low and B never reports. At 5 s the
  low battery warning for A shows. At 30 s the no reading warning (B) is due and waits until the
  low battery warning has ended, then shows yellow. If B reports in the meantime it never shows.

## Scenarios (without repeat options)

- B disconnects at 2 pm and stays away: one warning of about 10 seconds, then nothing. A layer
  change during the warning gives the single reminder blink, after the warning has ended it gives
  nothing.
- B disconnects, reconnects and disconnects again: two warnings.
- A's battery drops below the threshold at 3 pm: one low battery warning. After that only the
  battery keys show the level.
- The central or dongle reboots: all state is lost, so the boot warning and the events start over.
- B stays away and the user returns after a long break (more than `WARNING_INACTIVITY_MINUTES`):
  the first key press starts a new period, so B's missing state gives the boot warning again.

## Behavior changes

- Connection warnings no longer blink indefinitely: at most 30 seconds at boot, and the configured
  number of blinks after that.
- Battery warnings only consider required, connected peripherals. The required check uses the
  device number (`slot_number()`, from `SLOT_ORDER`), the same as the connection warning. Today
  `any_battery_low()` checks every slot, so it must skip slots whose device number is above the
  required limit and slots that are not connected.
- The connection warning keeps showing the device number of the highest-numbered missing required
  peripheral, with the same number of LEDs for every blink (no alternating count). If the strip has
  too few LEDs to show that number, all LEDs blink as a generic warning, as today.
  The battery warning still doesn't show which peripheral is low.
- `ZMK_LED_FEEDBACK_BATTERY_WARNING_REPEAT_MINUTES` is removed, so battery warnings no longer
  repeat while a battery stays low.

## Implementation notes

- Source: [src/led_feedback.c](../src/led_feedback.c) (warning state, timers, layer handling),
  [Kconfig](../Kconfig).
- Reuse the battery warning's blink counter and layer-dismiss logic for connection warnings rather
  than duplicating them. Ideally a single warning engine parameterized by type. The repeat timer
  and `warning_repeat_work` can be removed.
- Update the Kconfig help texts, [docs/connection-feedback.md](connection-feedback.md),
  [docs/battery-feedback.md](battery-feedback.md), the README, and the config validation tests
  in `tests/config-validation` (new option names, ranges, removed options).
- Update the firmware build test config in `tests/firmware-build/config/lf_test_dongle.conf`
  if it uses renamed options.

## Notes

- A battery level of `0` means no reading (existing behavior). It never counts as low. For a
  connected peripheral it is handled by the [no reading warning](#no-reading-warning).
- If a warning's condition resolves while it waits for a higher priority warning to end (for
  example a battery recovers or a reading arrives), it is dropped and doesn't show.
- The existing priority rules between the layer indication, display off and warnings stay as they
  are. Display off (and `OFF_OVERRIDES`) still decides whether warnings show, also in a new period
  after an inactivity reset: with the display off and no override, the new period runs but shows
  nothing.
