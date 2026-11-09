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

- Stage 3 adds diagnostic warning patterns. At this stage display-control behavior and the
  complete deadline handoff are not yet implemented, so use a display configured on for stage 3
  hardware checks.
- Continuously evaluate enabled feedback types in the specified priority order: missing required
  connections, any reported low battery below 15%, then missing required battery reports.
- Reuse existing connection indication and implement continuous low-battery and generic
  missing-report patterns with the specified timing and LED counts.
- Restart the selected pattern from its first phase when its type changes or feedback
  interruptions end. Preserve the specified always-on layer behavio{r.

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

For each test, add the listed settings to the dongle's `.conf` file, then build and flash the
dongle. These examples assume the LED feedback module and LED strip are already configured.
Remove or change any conflicting settings in the same file. Required peripherals must have
battery reporting enabled for their reports to reach the dongle.

1. **Connection warning**

    ```ini
    CONFIG_ZMK_LED_FEEDBACK_CONNECTION=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
    CONFIG_ZMK_LED_FEEDBACK_STARTUP_DIAGNOSTIC=y
    CONFIG_ZMK_LED_FEEDBACK_DISPLAY_OFF=n
    CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=-1
    CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=0
    ```

    Boot the dongle with a required peripheral powered off. Its red connection warning should
    appear during the first 20 seconds.

2. **Missing battery report**

    ```ini
    CONFIG_ZMK_LED_FEEDBACK_CONNECTION=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
    CONFIG_ZMK_LED_FEEDBACK_STARTUP_DIAGNOSTIC=y
    CONFIG_ZMK_LED_FEEDBACK_DISPLAY_OFF=n
    CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=-1
    CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=0
    ```

    Boot with a required peripheral powered off, then connect it within 20 seconds. Once all
    required peripherals are connected, yellow may appear while the dongle waits for battery
    reports. It should stop when all required reports arrive, provided no battery is below 15%.
    Reports may arrive too quickly to observe the yellow warning.

3. **Low-battery warning**

    ```ini
    CONFIG_ZMK_LED_FEEDBACK_CONNECTION=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
    CONFIG_ZMK_LED_FEEDBACK_STARTUP_DIAGNOSTIC=y
    CONFIG_ZMK_LED_FEEDBACK_DISPLAY_OFF=n
    CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=-1
    CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=0
    ```

    Boot with all required peripherals connected and at least one reporting a battery level
    below 15%. When the report arrives during the first 20 seconds, the diagnostic should show
    the repeating red/yellow warning.

    The diagnostic threshold is fixed at 15%. Setting
    `CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=100` changes **normal** warnings after the
    diagnostic; it does not make the diagnostic treat a healthy battery as low. If no peripheral
    reports below 15%, skip this hardware check. The automated test covers the 14% and 15%
    boundary using simulated reports.

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

Add the following to the dongle's `.conf`, replacing conflicting settings, and flash the
firmware. These tests assume the module and LED strip are already configured and the keymap
includes `LF_OFF`, `LF_ON`, `LF_TOG`, `LF_LAYER`, and an `&led_bat N` binding. Ensure a required
peripheral is disconnected so a diagnostic connection warning remains active.

```ini
CONFIG_ZMK_LED_FEEDBACK_CONNECTION=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
CONFIG_ZMK_LED_FEEDBACK_STARTUP_DIAGNOSTIC=y
CONFIG_ZMK_LED_FEEDBACK_DISPLAY_OFF=y
CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES="battery_feedback"
CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=-1
```

1. Boot the dongle. During the first 20 seconds, confirm the red connection warning is visible
   despite the configured display-off state. Request layer or battery feedback and confirm it
   temporarily interrupts the warning. The warning should restart afterward.
2. Reboot and press `LF_TOG` while the warning is visible. Confirm it turns the display off,
   since the temporary effective state was on. Request battery feedback; it should be visible
   due to the override, and the diagnostic warning should remain suppressed afterward.
3. Before 20 seconds have elapsed, press `LF_ON`. Confirm the applicable diagnostic warning
   resumes after any brightness preview. Repeat using `LF_OFF` to turn it off.

The complete deadline handoff remains stage 5; do not use these checks to validate it.

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
   f
