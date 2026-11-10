# Spec: Unified warnings (connection and battery)

Status: superseded by [spec-unified-warnings-no-repeat.md](spec-unified-warnings-no-repeat.md).
Backwards compatibility is not a concern, so options may be renamed or removed.

## Goal

Connection and battery warnings behave the same way. They share their timing and peripheral
options, warn once by default, can be repeated, and can be dismissed with a layer change.

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
   Disconnected peripherals are ignored by the battery warnings (the connection warning covers them).
4. **Warn once by default.** Every warning (connection, low battery, no reading) shows once per
   "event" (see [Warning lifecycle](#warning-lifecycle)) unless a repeat interval is configured.
   The boot warning is not a regular warning (see [Boot warning](#boot-warning-connection-only)).
5. **Separate repeat options.** Each warning type has its own `<TYPE>_WARNING_REPEAT_MINUTES`
   option. `0` means don't repeat (default). The no reading warning uses the battery option.
6. **Dismiss with a layer change.** A layer change dismisses an active warning of either type.
   One blink is shown after the layer indication clears if the condition still applies.

## Proposed options

### Shared (new / renamed)

| Option                                  | Default | Replaces                          | Notes                                              |
| --------------------------------------- | ------- | --------------------------------- | -------------------------------------------------- |
| `ZMK_LED_FEEDBACK_WARNING_BLINKS`       | `5`     | `BATTERY_WARNING_BLINKS`          | Range 1-1000. Used by both warnings. Name decided. |
| `ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS`    | `500`   | (existing, unchanged)             | Duration of each warning color.                    |
| `ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` | `-1`    | `CONNECTION_REQUIRED_PERIPHERALS` | Range -1-16, `-1` = all. Used by both.             |

### Per type

| Option                                               | Default | Notes                                                  |
| ---------------------------------------------------- | ------- | ------------------------------------------------------ |
| `ZMK_LED_FEEDBACK_CONNECTION`                        | `y`     | Existing bool, unchanged. Enables connection warnings. |
| `ZMK_LED_FEEDBACK_CONNECTION_WARNING_REPEAT_MINUTES` | `0`     | New. Range 0-1440. `0` = warn once.                    |
| `ZMK_LED_FEEDBACK_BATTERY_WARNING_REPEAT_MINUTES`    | `0`     | Existing.                                              |
| `ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL`             | `0`     | Existing (0 = off).                                    |

The pause between blinks is fixed at 1 second for both warnings (not an option). Total length of
a warning is roughly blinks x (2 x `BLINK_INTERVAL_MS` + 1 s): about 10 seconds with the defaults.
Mention this in the docs.

Removed: `ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS`, `ZMK_LED_FEEDBACK_CONNECTION_REQUIRED_PERIPHERALS`.

`ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` must be available when either connection or battery
feedback is enabled (`depends on ZMK_SPLIT`).

## Warning lifecycle

For each warning type, the condition is:

- Connection: `ZMK_LED_FEEDBACK_CONNECTION` is enabled and at least one required peripheral is not connected.
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
2. While the condition stays true:
    - repeat minutes is `0`: do nothing more.
    - repeat minutes is greater than `0`: show the warning again after that many minutes. The
      timer starts when the warning ends, so the minutes are the gap between two warnings.
3. The condition becomes false: the warning stops immediately, even in the middle of the blinks,
   and the state resets. The next time it becomes true it warns again.
4. Layer change while the warning is active: dismiss it (stop blinking, show the layer indication).
   After the layer indication clears, show one blink if the condition is still true.
   That blink is only a reminder: it is not a new warning and doesn't start or reset the repeat
   timer. A dismissed warning still follows the repeat rules above, and is not dismissed
   permanently.

### Boot warning (connection only)

Peripherals need time to connect after boot, so the connection warning starts with a boot phase:

1. At boot the connection warning starts immediately if a required peripheral is not connected.
   No delay and no option. Only runs when `ZMK_LED_FEEDBACK_CONNECTION` is enabled.
2. The boot warning is not a regular warning. It doesn't use up the "warn once" warning and the
   normal blink count doesn't apply. It blinks continuously (no pause, as today, because it is a
   different kind of warning) until every required peripheral is connected or 30 seconds have
   passed (fixed, not an option). Everyone who enables connection feedback and has missing
   peripherals at boot gets this warning.
3. All required peripherals connected within 30 seconds: the boot warning stops and the lifecycle
   above starts with a clean state. A later disconnect gives the regular warning (once).
4. 30 seconds passed with a required peripheral still missing: the boot warning stops and the
   condition counts as a warning that was shown. After that only the repeat rules apply. With
   repeat set to `0` the user is not warned again (intended). The docs should tell users who want
   reminders to set `CONNECTION_WARNING_REPEAT_MINUTES`.
5. A layer change during the boot warning interrupts it (the layer indication has priority). After
   the layer indication clears it resumes blinking continuously for the rest of the 30 seconds,
   and can be interrupted again. There is no reminder blink. The 30 seconds keep counting during
   the layer indication.

## Warning priority

The warning types keep independent state and timers, but never show at the same time:

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
- The repeat timer of a warning starts when that warning has actually ended. Time spent waiting
  for a higher priority warning doesn't count towards the repeat interval.
- This includes the boot warning: battery warnings wait until the boot warning ends.

## No reading warning

A battery warning variant for when `BATTERY_WARNING_LEVEL` is above 0 and one or more required
peripherals are connected but don't report a battery level. A battery that can't be read can't be
confirmed healthy.

- Blinks yellow only (the low battery warning blinks red then yellow). It is part of the battery
  warning, not a third warning type: same `WARNING_BLINKS`, `BLINK_INTERVAL_MS`, fixed pause,
  `BATTERY_WARNING_REPEAT_MINUTES`, layer change dismissal, stop on resolve and event definition.
  No new options. The low battery and no reading warnings each have their own repeat timer, using
  the same number of minutes, so one doesn't restart the other's repeat.
- Disconnected peripherals are ignored, the connection warning covers them.
- Shows immediately when it is due, and only when no connection warning is active (or after it has
  ended). It lasts for the `WARNING_BLINKS` period, then follows the repeat rules.
- Readings arrive some time after a peripheral connects, so the condition is only evaluated once
  30 seconds have passed since boot (the same fixed window as the boot warning). A peripheral
  that connects later may give a short warning until its first reading arrives (accepted).
- When a reading arrives or the peripheral disconnects, the condition is false and the warning
  stops immediately.
- If a peripheral has no reading and another is low, the no reading warning shows first. The low
  battery warning follows when it ends, if still low. Exception: the no reading warning isn't due
  in the first 30 seconds, so a low battery warning isn't delayed by it.
- Example: both peripherals connect 5 seconds after boot, A is low and B never reports. At 5 s the
  low battery warning for A shows. At 30 s the no reading warning (B) is due and waits until the low
  battery warning has ended, then shows yellow. If B reports in the meantime it never shows.

## Behavior changes

- Connection warnings no longer blink indefinitely: at most 30 seconds at boot, and the configured
  number of blinks after that.
- Battery warnings only consider required peripherals. The required check uses the device number
  (`slot_number()`, from `SLOT_ORDER`), the same as the connection warning. Today
  `any_battery_low()` checks every slot, so it must skip slots whose device number is above the
  required limit.
- The connection warning keeps showing the device number of the highest-numbered missing required
  peripheral, with the same number of LEDs for every blink (no alternating count). If the strip has
  too few LEDs to show that number, all LEDs blink as a generic warning, as today.
  The battery warning still doesn't show which peripheral is low.

## Implementation notes

- Source: [src/led_feedback.c](../src/led_feedback.c) (warning state, timers, layer handling), [Kconfig](../Kconfig).
- Reuse the battery warning's blink counter, repeat timer and layer-dismiss logic for connection
  warnings rather than duplicating them. Ideally a single warning engine parameterized by type.
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
- The existing priority rules between the layer indication, display off and warnings stay as they are.

## Open questions

1. Boot warning state: when it ends with a peripheral still missing it counts as the shown
   warning. Suggested: that state resets as soon as all required peripherals are connected at
   the same time, so a later drop is a new event.
2. Layer change after a warning has already finished its blinks: is there a reminder blink?
   Suggested: no, the reminder blink only follows an interrupted (active) warning.
