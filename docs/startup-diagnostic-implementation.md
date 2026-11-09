# Startup diagnostic implementation stages

Implement the [startup diagnostic specification](startup-diagnostic-spec.md) as one cohesive
feature, using the stages below to keep changes understandable and testable. Validate each stage
before proceeding; the feature is not complete until the integrated behavior is verified.

Automated tests and firmware builds are developer checks. Hardware checks are optional manual
tests on the dongle and peripherals. Later-stage tests are plans, not tests that can necessarily
be run before that stage is implemented.

## 1. Share the required-peripherals setting

- Rename `CONFIG_ZMK_LED_FEEDBACK_CONNECTION_REQUIRED_PERIPHERALS` to
  `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` and make it available when connection or battery
  feedback is enabled.
- Preserve the existing required-device count and slot-order semantics for connection feedback.
- Update references in code, tests, and directly related documentation. Backward compatibility
  with the old option name is not required.

### Stage 1 tests

**Automated checks**

1. Build with the default required-peripheral setting; verify it is `-1`.
2. Build with connection feedback enabled and the required count set to `0`; verify the
   generated configuration retains `0`.
3. Build with battery feedback enabled and connection feedback disabled; verify the shared
   setting can be set to `1`.
4. Build with a required count of `1` and slot order `2,1`; verify both settings are retained.
5. Run the existing configuration-validation tests.

**Hardware checks**

1. With the required count unset, power off each peripheral in turn. Its connection warning
   should appear and stop when it reconnects.
2. Set the required count to `1`. Device 1 should produce a warning when disconnected; device
   2 should not.
3. Set the required count to `0`. Disconnecting either device should not produce a warning.
4. If using a custom slot order, check that the required device is selected by its configured
   device number rather than its pairing slot.

## 2. Add diagnostic state and timing

- Add the opt-in `CONFIG_ZMK_LED_FEEDBACK_STARTUP_DIAGNOSTIC` setting.
- Introduce a fixed 20-second diagnostic window beginning at central boot, with no grace period.
- Keep diagnostic activity separate from normal warning state; retain observed connection and
  battery data across the eventual handoff.

Stage 2 establishes the boot-relative window only. It does not yet display diagnostic warnings,
change the display state, or perform a handoff; normal feedback continues even if the option is
enabled. The window is evaluated from system uptime, so it is independent of status events.

### Stage 2 tests

**Automated checks**

1. Run the timing unit test in
   [`tests/startup-diagnostic`](../tests/startup-diagnostic/). It substitutes a controlled
   uptime for the clock and checks that the window is active at 19,999 milliseconds and
   inactive at 20,000 milliseconds and afterward. It does not wait 20 seconds or require a
   dongle.
2. Build with the diagnostic disabled; verify the option is off by default.
3. Build with the diagnostic and connection feedback enabled; verify the option is enabled.
4. Build with the diagnostic and battery feedback enabled but connection feedback disabled;
   verify the option is enabled.

**Hardware checks**

1. With the diagnostic disabled, disconnect and reconnect a required peripheral. Confirm normal
   connection warnings still work.
2. Enable `CONFIG_ZMK_LED_FEEDBACK_STARTUP_DIAGNOSTIC=y`, rebuild, and flash the dongle.
   Disconnect and reconnect a required peripheral before 20 seconds after boot, then repeat
   after 20 seconds. Normal connection warnings should work in both cases.
3. If battery feedback is enabled, request battery readings with the battery keycodes and
   confirm they still work.

The window has no visible indication at this stage, so hardware observation alone cannot verify
its deadline. Diagnostic warning and handoff tests belong to later stages.

## 3. Implement warning selection and patterns

- Continuously evaluate enabled feedback types in the specified priority order: missing required
  connections, any reported low battery below 15%, then missing required battery reports.
- Reuse existing connection indication and implement continuous low-battery and generic
  missing-report patterns with the specified timing and LED counts.
- Restart the selected pattern from its first phase when its type changes or feedback
  interruptions end. Preserve the specified always-on layer behavior.

### Stage 3 tests

**Automated checks**

1. Check priority: a missing required connection takes precedence over low battery, which takes
   precedence over a missing required report.
2. Check the threshold: a 14% report produces a low-battery warning; 15% does not. Verify that
   a low report from an optional peripheral also produces a warning.
3. Check missing reports with all required reports present, one missing, no reports, zero required
   peripherals, and a reordered slot list.
4. Check connection-only, battery-only, and combined feedback configurations.
5. Change status during a pattern and verify the newly selected warning starts at its first
   phase. Verify normal battery blink-count and repeat settings do not limit diagnostic patterns.
6. Check the configured always-on layer appears when no warning is active and in connection
   blink gaps; battery-warning gaps stay dark.

**Hardware checks**

1. With both feedback types enabled, boot with a required peripheral disconnected. Confirm its
   red connection warning appears.
2. Connect it and observe the battery diagnostic: yellow may appear while reports are missing,
   then stop when healthy reports arrive.
3. If a low-battery report is available, confirm the red/yellow warning appears. Use controlled
   test reports for threshold boundary cases rather than relying on a battery reaching a
   particular level.

## 4. Integrate display controls and interruptions

- Implement the temporary effective display-on state without permanently changing the user's
  display preference.
- Apply `LF_ON`, `LF_OFF`, `LF_TOG`, and brightness key behavior as specified; preserve normal
  display-off overrides for other feedback.
- Allow battery and layer feedback requests, layer changes, and brightness previews to interrupt
  diagnostics. Re-evaluate the current status when they finish.

### Stage 4 tests

**Automated checks**

1. Start with the display configured off. Verify it is effectively on during diagnostics and
   permits ordinary battery and layer feedback without off overrides.
2. Exercise `LF_ON`, `LF_OFF`, `LF_TOG`, and both brightness keys; verify the effective and
   user-selected states follow the specification.
3. With the display explicitly off, verify diagnostic warnings remain suppressed. Allow battery
   feedback with its off override, then confirm the warning remains suppressed afterward.
4. Interrupt a warning with battery feedback, a layer request, a layer change, and a brightness
   preview. After each finishes, verify the current warning is re-evaluated and restarted.

**Hardware checks**

1. Configure the display to start off and boot the dongle. Confirm diagnostic warnings and
   ordinary feedback can be visible during the diagnostic.
2. Press `LF_OFF` during a warning and confirm it stops. With a battery-feedback off override,
   request a battery reading and confirm the diagnostic warning does not return afterward.
3. Turn the display back on and confirm an applicable diagnostic warning resumes.

## 5. Implement the deadline handoff

- Stop in-progress diagnostic warnings and feedback displays at the deadline. Prevent queued or
  running diagnostic work from writing or rescheduling after normal feedback takes over.
- Preserve observed connection and battery status, current layer and layer-toggle state, and
  user-selected display and brightness settings.
- Restore the applicable normal display state and start normal checks afresh using the retained
  status and configured warning and repeat settings. Missing-report warnings are diagnostic-only.

### Stage 5 tests

**Automated checks**

1. At the deadline, verify in-progress diagnostic and explicit feedback stop, observed connection
   and battery status remain available, and missing-report warnings do not resume.
2. With a retained low report, verify normal warnings and configured repeats start according to
   the normal threshold. Repeat with the normal threshold set to `0`.
3. Repeat the low-battery handoff with the display on, off with a battery-warning override, and
   off without that override. Verify normal visibility and repeat rules apply.
4. Change the layer, `LF_LAYER_TOG`, brightness, and display setting during the diagnostic.
   Verify handoff preserves those choices and restores the appropriate always-on layer.
5. Arrange diagnostic work and handoff near the deadline, including work already running.
   Verify no diagnostic update can overwrite normal feedback or reschedule after handoff.

**Hardware checks**

1. Start with the display off and make no display-control changes. Confirm it returns off at
   handoff; repeat using `LF_ON` and confirm it remains on.
2. With a required peripheral disconnected, observe the transition to normal configured
   connection warnings at the deadline.
3. If a low-battery report is available, confirm normal warning and display-off settings apply
   after handoff.

## 6. Document and validate the integrated feature

- Update user-facing configuration and behavior documentation, including the renamed option,
  diagnostic prerequisites, temporary display behavior, and handoff.
- Extend the firmware build matrix to cover relevant configuration combinations. Add focused
  runtime behavior tests: the existing configuration-validation tests and firmware builds alone
  do not verify warning behavior.

### Stage 6 tests

**Automated checks**

1. Run all focused tests added during the preceding stages.
2. Build the relevant firmware configuration combinations, including diagnostic disabled,
   connection-only, battery-only, and both feedback types enabled.
3. Verify the complete diagnostic-to-normal handoff and the diagnostic-disabled path.

**Hardware checks**

1. Exercise a complete startup with the intended dongle and peripherals. Observe how long
   connection and battery reports take and whether warnings and handoff match the specification.
2. Repeat with the diagnostic disabled and check that normal feedback behaves as before.
3. Record any scenarios that could not be tested on hardware.
