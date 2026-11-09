# Startup diagnostic implementation stages

Implement the [startup diagnostic specification](startup-diagnostic-spec.md) as one cohesive
feature, using the stages below to keep changes understandable and testable. Validate each stage
before proceeding; the feature is not complete until the integrated behavior is verified.

## 1. Share the required-peripherals setting

- Rename `CONFIG_ZMK_LED_FEEDBACK_CONNECTION_REQUIRED_PERIPHERALS` to
  `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` and make it available when connection or battery
  feedback is enabled.
- Preserve the existing required-device count and slot-order semantics for connection feedback.
- Update references in code, tests, and directly related documentation. Backward compatibility
  with the old option name is not required.
- Verify connection behavior and configuration combinations, including battery-only and zero
  required peripherals. With the diagnostic disabled, existing runtime behavior must remain
  unchanged.

## 2. Add diagnostic state and timing

- Add the opt-in `CONFIG_ZMK_LED_FEEDBACK_STARTUP_DIAGNOSTIC` setting.
- Introduce a fixed 20-second diagnostic window beginning at central boot, with no grace period.
- Keep diagnostic activity separate from normal warning state; retain observed connection and
  battery data across the eventual handoff.
- Verify that the deadline does not move when status changes and that the disabled path retains
  existing behavior.

## 3. Implement warning selection and patterns

- Continuously evaluate enabled feedback types in the specified priority order: missing required
  connections, any reported low battery below 15%, then missing required battery reports.
- Reuse existing connection indication and implement continuous low-battery and generic
  missing-report patterns with the specified timing and LED counts.
- Restart the selected pattern from its first phase when its type changes or feedback
  interruptions end. Preserve the specified always-on layer behavior.
- Test boundary values, missing and optional peripherals, slot ordering, warning transitions,
  feedback types enabled independently and together, and normal warning settings having no
  effect on diagnostic pattern duration.

## 4. Integrate display controls and interruptions

- Implement the temporary effective display-on state without permanently changing the user's
  display preference.
- Apply `LF_ON`, `LF_OFF`, `LF_TOG`, and brightness key behavior as specified; preserve normal
  display-off overrides for other feedback.
- Allow battery and layer feedback requests, layer changes, and brightness previews to interrupt
  diagnostics. Re-evaluate the current status when they finish.
- Test display-on and display-off startup, toggling the effective state, overrides, and
  interruptions, including that an explicit off keeps diagnostic warnings suppressed.

## 5. Implement the deadline handoff

- Stop in-progress diagnostic warnings and feedback displays at the deadline. Prevent queued or
  running diagnostic work from writing or rescheduling after normal feedback takes over.
- Preserve observed connection and battery status, current layer and layer-toggle state, and
  user-selected display and brightness settings.
- Restore the applicable normal display state and start normal checks afresh using the retained
  status and configured warning and repeat settings. Missing-report warnings are diagnostic-only.
- Test a low battery at handoff with the display on and off, with and without the battery-warning
  override; verify the normal repeat behavior and restoration of the always-on layer.
- Test interruptions and diagnostic work near the deadline, including simultaneous or
  already-running work, so diagnostic feedback cannot reappear afterward.

## 6. Document and validate the integrated feature

- Update user-facing configuration and behavior documentation, including the renamed option,
  diagnostic prerequisites, temporary display behavior, and handoff.
- Extend the firmware build matrix to cover relevant configuration combinations. Add focused
  runtime behavior tests: the existing configuration-validation tests and firmware builds alone
  do not verify warning behavior.
- Run the smallest relevant tests during each stage, then run integrated validation covering the
  complete diagnostic and diagnostic-disabled paths.
- Exercise the startup sequence on actual dongle and peripheral hardware if available, including
  connection and battery-report timing. Report any validation that could not be performed.
