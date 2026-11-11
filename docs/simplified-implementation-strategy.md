# Simplified implementation strategy

## Purpose

Implement [the simplified specification](simplified-spec.md) in stages. The specification is the source of truth for behavior; this document describes the order of work and the checks between stages. Backward compatibility is not required.

Rewrite warning arbitration and scheduling within the existing module. Reuse suitable rendering, ZMK integration, behaviors, and configuration validation, but remove obsolete warning paths rather than running old and new implementations side by side. If the warning code cannot be separated cleanly from retained feedback, stop and reassess a full rewrite before expanding scope.

Each stage has a gate. Do not rely on a passing configuration-only test as proof that runtime warnings work, or on a firmware build as proof that timing behavior is correct. Add or adjust tests with the implementation and resolve failures in the current stage before moving on. Preserve unrelated worktree changes.

## Existing test infrastructure

- `tests/config-validation` is a host CMake/CTest fixture. It checks the module's CMake validation, but it does not exercise ZMK's Kconfig processing, devicetree, or warning runtime.
- `.github/workflows/firmware-build.yml` builds a ZMK v0.3 test dongle with `tests/firmware-build/config` across the four warning combinations and runs real ZMK configuration validation.
- `tests/warning-state` exercises the internal warning state machine and, with `tests/warning-runtime`, the real runtime handler through simulated Zephyr events, work scheduling, and LED output. It does not replace a real firmware build or hardware test. The suite also runs in the configuration-validation CI workflow.

## Stage 0: Establish a baseline

1. Review the current worktree and the code paths for connection tracking, battery events, warning work, strip updates, display controls, and layer feedback.
2. Record the results of the existing configuration tests and a representative firmware build if the ZMK toolchain is available. Existing failures are baseline information, not an instruction to fix unrelated issues.
3. Identify the retained behavior bindings, hooks, rendering helpers, and documentation that must continue to work. Confirm the runtime hook count remains clamped even though invalid configured LED counts will fail the build.

The current host-test commands, from the module root, are:

```sh
cmake -S tests/config-validation -B build/config-validation
ctest --test-dir build/config-validation --output-on-failure
```

**Gate:** baseline results and any unavailable prerequisites are recorded. The current module boundary is understood well enough to begin replacing warning behavior without removing retained features.

### Stage 0 baseline

Recorded on the `simplified` branch before implementation changes:

- The existing CMake/CTest configuration suite passed all 22 tests. CMake 3.31.6 emitted a developer warning about policy `CMP0174` and an empty `LAYER_COLORS` argument; the tests still passed.
- The test dongle firmware built successfully for `nice_nano_v2` with ZMK `v0.3`, Zephyr 3.5.0, and Zephyr SDK 0.16.9. Its effective configuration enabled split-central mode, the module, connection feedback, battery feedback, and a 15% battery-warning threshold. The build produced a UF2; its temporary container build directory was not retained.
- The firmware build emitted existing deprecation warnings for the fixture's `config/boards` layout and `NRF_STORE_REBOOT_TYPE_GPREGRET`. The first build attempt could not find Zephyr's CMake package in a fresh container; supplying `Zephyr_DIR` as below resolved it.
- The fixture uses a mock key scanner and an LED-strip overlay. A successful build establishes compilation, not physical LED behavior or coverage of every key binding. Hardware timing and visual checks remain for Stage 5.
- The worktree had the spec and this strategy as untracked files; no existing source changes were reverted or modified for the baseline.

The successful firmware baseline used the existing West workspace and a local Docker image. Run from the module root; mounts are read-only and the build is temporary:

```sh
docker run --rm --entrypoint sh \
	-v "$HOME/github/zmk-build-workspace:/workspace:ro" \
	-v "$PWD:/module:ro" -w /workspace \
	zmkfirmware/zmk-dev-arm:4.1-branch -c '
		west build -s /workspace/zmk/app -d /tmp/lf-stage0-build -b nice_nano_v2 -- \
			-DZephyr_DIR=/workspace/zephyr/share/zephyr-package/cmake \
			-DZMK_CONFIG=/module/tests/firmware-build/config \
			-DZMK_EXTRA_MODULES=/module -DSHIELD=lf_test_dongle \
			-DCONFIG_ZMK_DISPLAY=n -DCONFIG_ZMK_LED_FEEDBACK_BATTERY=y \
			-DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=15
	'
```

The implementation boundary identified for the next stages is:

- `src/led_feedback.c` owns strip updates, brightness, display-off checks, layer scheduling, connection callbacks and blink work, battery events and display work, and battery-warning scheduling. Connection warnings currently defer to layer or requested battery displays; layer handling can dismiss a battery warning and schedule a reminder. These paths must be replaced or adapted together, not left competing with the new state machine.
- The current required-peripheral count and connection tracking are guarded by connection feedback, while battery warnings scan battery slots separately. Shared filtering and battery-only warning configurations therefore need independent coverage.
- `draw_layer()` clamps the result of the weak layer LED-count hook to `0..STRIP_LEN`. Retain that runtime behavior and the layer color hook and helpers while enforcing the new build-time limits for configured counts.
- The two behavior drivers, their devicetree bindings, `include/dt-bindings/zmk/led_feedback.h`, and `include/zmk_led_feedback.h` provide the existing display, brightness, layer, and battery controls. Preserve the agreed user-facing capabilities and test their integration, even though compatibility of names and APIs is not required.
- The README and the existing connection, battery, display-off, keycode, layer, and peripheral-numbering documentation describe the current behavior and will need review in Stage 5.

**Stage 0 result:** gate met for baseline tests, representative firmware compilation, and implementation-boundary review. Hardware behavior was not tested.

## Stage 1: Isolate warning decisions

1. Introduce a small warning state machine with an injected or controllable monotonic clock. Keep it separate from LED rendering and ZMK callbacks where practical.
2. Model the fixed 20-second boot window, independent warning enablement, shared required-peripheral selection, processed-event cutoff, connection priority, and one latched aggregate low-battery warning. Use the already processed connection state when boot closes; late events may be missed for warnings but must still update ordinary state.
3. Model minimum connection blinks and post-deadline eligibility, full battery-warning restart after connection interruption, and cancellation when the display is turned off. Keep the display-off gate above every feedback source.
4. Add deterministic tests for both event orders, repeated reports, deadline boundaries, late deadline work, reconnects, and display toggles. Test the actual decision logic, not a second implementation written only for tests.

**Gate:** focused state-machine tests pass with a fake clock, including events just before, at, and after the deadline; a late-processed event; a late second disconnection; and battery-warning restart or cancellation. No real-time sleeps are needed.

### Stage 1 result

- Added `src/warning_state.c` and its private header as a plain C warning decision engine. It takes explicit monotonic times, tracks the shared required-peripheral mapping and processed boot state, reports warning output and the next deadline, and returns failure for invalid internal configuration. It is not a public module API.
- The caller must serialize state access, provide a valid full slot-to-device mapping and configured timing values, check the initialization result, and schedule the next deadline returned by the engine. A late wake processes the next phase rather than inventing LED blinks that were not displayed. Integrating these responsibilities with ZMK callbacks, strip output, display controls, and work cancellation remains Stage 2.
- Added 21 named deterministic scenarios under `tests/warning-state`, covering independent warning options, mapped and optional peripherals, the 20-second cutoff, delayed events and work, connection priority and minimum blinks, battery sequences and restarts, post-boot state updates, and display-off cancellation. The suite passes with compiler warnings treated as errors and also passes with address and undefined-behavior sanitizers.
- The existing 22 configuration-validation tests still pass, with the Stage 0 `CMP0174` developer warning unchanged. The state-machine source also compiles with the Zephyr SDK ARM compiler. The updated CI workflow includes the new host suite.
- At the end of Stage 1 the engine was deliberately not yet wired into firmware; runtime integration and configuration changes were deferred to Stages 2 and 3. No hardware behavior was tested in Stage 1.

Run the new host suite from the module root:

```sh
cmake -S tests/warning-state -B build/warning-state
cmake --build build/warning-state
ctest --test-dir build/warning-state --output-on-failure
```

**Stage 1 result:** gate met for isolated warning decisions and deterministic host tests. Integrated firmware and hardware validation remain later-stage work.

## Stage 2: Replace runtime integration

1. Connect the state machine to the existing ZMK battery listener, Bluetooth connection tracking, monotonic clock, and Zephyr work scheduling. Serialize state changes and cutoff handling so later events cannot enter the boot-warning set. Verify initialization order and avoid losing early connections; choose the mechanism during implementation.
2. Replace the old connection, battery-warning, repeat, and layer-interruption scheduling paths. Cancel stale work on transitions so old handlers cannot overwrite a newer output or revive a completed warning.
3. Route warning rendering through the existing strip abstraction: connection red blinks, generic red/yellow battery sequence, configured blink timings, and no-reading behavior. Treat zero as unavailable, including for on-demand battery display.
4. Apply display arbitration: off suppresses all output; warnings preempt ordinary feedback; requests and previews interrupted by or made during warnings are discarded; brightness controls still update brightness and turn the display on. Restore continuous layer feedback only after all eligible warnings finish.
5. Keep battery and connection tracking needed for on-demand feedback up to date after boot, even though post-deadline events cannot create warnings. The required-peripheral filter must work for battery warnings even when connection warnings are disabled.

**Gate:** rerun the state tests against the integrated control path or a faithful adapter, including LED phases and canceled work. Build a representative ZMK v0.3 firmware with both warnings enabled. Verify retained controls and no-warning behavior, including layer feedback, brightness, battery requests, display off/on, slot mapping, and hooks. If the existing abstractions prevent a clean replacement, stop and reassess the implementation approach.

### Stage 2 result

- `src/led_feedback.c` now routes connection and battery events, warning deadlines, display state, and LED warning phases through the Stage 1 engine. The former repeating, reminder, connection-blink, and battery-warning scheduling paths have been removed. Warning-state access and LED-strip writes are serialized separately; early connection state is retained before engine initialization.
- The required-peripheral setting is available with connection warnings disabled, and the runtime uses it for both warning types. On-demand battery levels remain updated after boot. Layer changes and on-demand displays cannot interrupt warnings; display-off cancellation and brightness controls use the same warning state.
- The host suite passes 32 deterministic cases, including tests that compile the actual runtime handler against simulated Zephyr events, work queues, LED output, and layer hooks. It also passes under address and undefined-behavior sanitizers. The existing 22 configuration-validation tests pass with the previously recorded `CMP0174` developer warning.
- ZMK v0.3 firmware builds passed with both warning types, battery-only, connection-only, neither, and layer feedback disabled; effective settings were checked. The firmware fixture now instantiates both LED behaviors. A separate build of the actual Lily58 dongle, keymap, and layer-hook module also passed.
- No hardware tests were performed. VS Code's local C/C++ diagnostics still report missing Zephyr include paths; the Docker firmware builds use the real Zephyr toolchain and passed.
- At the end of Stage 2, configuration migration and strict build-time validation remained for Stage 3. Invalid legacy settings could cause warning initialization to fail at runtime rather than fail the build.

**Stage 2 result:** runtime integration gate met for deterministic host tests and representative firmware builds. Hardware verification remains Stage 5; configuration migration and validation remain Stage 3.

## Stage 3: Configuration and validation

1. Implement independent boot-only connection and battery-warning opt-ins with the agreed defaults. Remove the startup-display-off, display-off override, and battery-repeat options and their build validation; do not retain compatibility aliases.
2. Make required-peripheral selection shared between warnings. Validate slot mappings as an empty identity mapping or a full permutation of configured slots, and reject invalid required counts.
3. Validate threshold `0` or `2-100`, blink interval `100-1000` ms, battery blink count `1-10`, ordered brightness limits and initial brightness, and applicable per-layer LED counts against strip length. Reject invalid values with actionable errors; preserve runtime brightness and hook bounds.
4. Extend the existing configuration fixture for module CMake validation, replacing deleted override cases. Add checks in an actual ZMK/Zephyr configuration or firmware build for Kconfig and devicetree-dependent constraints, including strip length. Verify that invalid user values actually fail the build rather than being silently normalized or replaced by Kconfig.

**Gate:** valid boundary values pass and invalid formats, duplicates, out-of-range values, and invalid cross-option combinations fail with the expected error. Exercise all four warning configurations: neither, connection only, battery only, and both. Run the existing host CTest suite and the targeted real ZMK configuration/build checks.

### Stage 3 result

- Connection warnings now default off, battery warnings remain independently opt-in, and `CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS` filters both warning types. The former connection-specific required-count name and obsolete startup-off, override, and repeat options were removed without aliases.
- Applicable values and complete slot permutations are validated at build time. Brightness ordering and configured layer LED counts are also checked, with the latter checked against the actual devicetree strip length. Runtime brightness and layer-hook results retain their bounds.
- The host configuration suite passed 50 tests; the warning state and simulated runtime suite passed 32 tests. The real ZMK configuration suite passed 39 scenarios, including valid boundaries, rejected settings and legacy names, and actual strip-length builds. The firmware CI workflow now runs this suite and builds the four warning combinations.
- Full ZMK v0.3 test-dongle builds passed with neither warning, connection only, battery only, and both; effective warning settings were verified. All seven complete Lily58 dongle profiles and five awake, sleep, and no-report peripheral builds also passed, with every explicitly requested profile setting checked against the generated configuration. These builds used temporary copies and did not flash hardware.
- Physical LED behavior and timing have not been tested. The existing user-facing README and guides still need the Stage 5 documentation update.

To run the real ZMK configuration suite with the same local Docker workspace:

```sh
docker run --rm --entrypoint sh \
	-v "$HOME/github/zmk-build-workspace:/workspace:ro" \
	-v "$PWD:/module:ro" \
	-e ZMK_WORKSPACE=/workspace \
	-e MODULE_SOURCE_DIR=/module \
	-e ZMK_CONFIG_DIR=/module/tests/firmware-build/config \
	-e VALIDATION_BUILD_ROOT=/tmp/lf-real-config-validation \
	-w /workspace zmkfirmware/zmk-dev-arm:4.1-branch \
	-c 'sh /module/tests/firmware-build/validate_config.sh'
```

**Stage 3 result:** configuration and validation gate met. Broader regression coverage and physical hardware verification remain in Stages 4 and 5.

## Stage 4: Firmware and regression matrix

1. Update the firmware CI matrix to remove deleted options and cover the four warning configurations. Include battery monitoring without warnings and connection-disabled battery warnings, so feature dependencies are checked independently.
2. Exercise representative split configurations: zero, some, and all required slots; identity and reordered slot mappings; optional peripherals; a strip shorter than the configured slot count; and available layer and battery feedback combinations. Add or adapt fixtures only where existing ones cannot cover the case.
3. Build retained behavior bindings and hooks with the new implementation. Exercise display-off/on, brightness, layer changes, and requested battery displays outside warnings and during both types of warnings.
4. Extend automated tests for the specification's exact edge cases: zero and unavailable readings, low reports that later recover, post-boot state updates, repeated interruptions, the two-blink minimum, reconnects during spillover, ignored late disconnections, and no warning replay after display-off cancellation.

**Gate:** deterministic runtime tests, configuration tests, and the updated firmware matrix pass. Verify the actual chosen Kconfig values and build errors in firmware builds, not merely that the build command returned success. Any unavailable matrix case or toolchain must be reported explicitly.

## Stage 5: Documentation and hardware validation

1. Update the README and existing connection, battery, display-off, keycode, and peripheral-numbering documentation to match the implemented configuration and behavior. Remove obsolete override, startup-off, repeat, and layer-override instructions. Keep the simplified spec and this strategy consistent with any approved implementation decisions.
2. Explain the 20-second detection window, minimum connection warning, possible battery-warning spillover, display-off cancellation, shared required-peripheral filter, and the fact that missing or zero battery readings do not trigger a warning. Document the single no-reading LED and the possibility that a genuine 0% reading is treated as unavailable.
3. On representative hardware, follow the specification's timing-validation plan. Record module initialization, required peripheral connections, and first valid battery reports across multiple cold starts, sleeping and already-awake peripherals, staggered power-on, and reconnects. Record late or missing reports as well as successful timings.
4. Observe the LED sequences and user controls on hardware, including a late connection warning, its minimum blinks, connection-to-battery priority, a battery-warning restart, and display-off cancellation. Test the ordinary layer and battery workflows after boot.

Use the complete configurations and step-by-step procedures in [the hardware test plan](simplified-hardware-tests.md). Verify every profile builds with the effective settings before hardware testing.

**Gate:** documentation matches the final implementation, automated checks remain green, and hardware observations are recorded. If hardware or a required toolchain is unavailable, state which checks remain unverified and provide the commands or procedure needed to complete them; do not report those checks as passed.

## Completion criteria

- All acceptance cases in the [simplified specification](simplified-spec.md#acceptance-tests) are covered by an appropriate automated test, firmware build, or explicitly recorded hardware check.
- Old warning code and removed options are gone; retained module features and user controls work under the new arbitration rules.
- Host tests and the representative ZMK firmware matrix pass, with no unrelated regressions introduced.
- Hardware timing and visual checks are completed, or the remaining limitations and exact follow-up are documented.
- Any implementation-discovered behavior decisions or conflicts are recorded in the spec before being treated as settled.
