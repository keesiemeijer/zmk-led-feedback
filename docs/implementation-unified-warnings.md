# Implementation strategy: Unified warnings

Implements [spec-unified-warnings-no-repeat.md](spec-unified-warnings-no-repeat.md). The spec
decides behavior, this file decides order and verification.

## Starting point

- All logic is in [src/led_feedback.c](../src/led_feedback.c) (about 900 lines). Connection and
  battery warnings are two separate state machines: `blink_work`/`blink_handler` (connection,
  continuous) and `battery_work`/`warning_work`/`warning_repeat_work` (battery, N blinks, repeat).
- Options are in [Kconfig](../Kconfig). Build-time checks are in
  [CMakeLists.txt](../CMakeLists.txt) (text options) and `BUILD_ASSERT` in `led_feedback.c`
  (numbers).
- Tests available:
  - `tests/config-validation`: CMake/ctest cases for the CMake checks only. It cannot test
    Kconfig ranges or `BUILD_ASSERT`.
  - `tests/firmware-build`: real ZMK firmware build in CI (`lf_test_dongle.conf`). It proves the
    code compiles and the Kconfig is valid, not the runtime behavior.
- There is no automated runtime test for the blinking. Behavior is verified on hardware (or
  `native_posix` if available), so each stage ends with a manual checklist.

## Build and test commands

Run from `/Users/keesmeijer/github` with Docker running. The workspace is
`zmk-build-workspace` (ZMK v0.3, board `nice_nano_v2`, shield `lf_test_dongle`), the module is
mounted at `/module`. Use a new build dir per stage (`build/stageN`).

```sh
# Firmware build
docker run --rm -v "$PWD/zmk-build-workspace":/workspace -v "$PWD/zmk-led-feedback":/module \
  -w /workspace zmkfirmware/zmk-dev-arm:4.1-branch bash -c \
  'west zephyr-export >/dev/null 2>&1; west build -s zmk/app -d build/stage0 -b nice_nano_v2 -- \
   -DSHIELD=lf_test_dongle -DZMK_CONFIG=/workspace/config -DZMK_EXTRA_MODULES=/module'

# Config validation tests (cmake is not installed on the host)
docker run --rm -v "$PWD/zmk-led-feedback":/module -w /module zmkfirmware/zmk-dev-arm:4.1-branch \
  bash -c 'cmake -S tests/config-validation -B /tmp/cv >/dev/null && ctest --test-dir /tmp/cv'
```

Note: `west zephyr-export` is needed first, or CMake can't find Zephyr. The workspace
`config/lf_test_dongle.conf` is a copy of the one in this repo, keep them in sync when options are
renamed.

Stage 0 result (branch `warning-types`): firmware build passes (FLASH 210536 B, 25.96%), 22 of
22 config validation tests pass.

## Principles

- One stage = one commit, and the firmware build passes after every stage.
- Stages 1-2 should not change user visible behavior except where named. Behavior changes arrive
  stage by stage, each with a checklist.
- The shared warning engine replaces the two state machines. Don't keep both.
- Update docs in the same stage as the behavior they describe.

## Stages

### Stage 0: Baseline

- Run the config validation tests and the firmware build on the current `main` to know they pass.
- Write down the current behavior checklist on hardware (connection blinks, battery blinks,
  layer dismissal, battery repeat) so regressions can be told apart from intended changes.

Test: `ctest` in `tests/config-validation`; firmware build workflow.

### Stage 1: Options (Kconfig only)

- Add `WARNING_BLINKS` (1-1000, default 5), `REQUIRED_PERIPHERALS` (-1-16, default -1),
  `WARNING_INACTIVITY_MINUTES` (0-1440, default 0).
- Remove `BATTERY_WARNING_BLINKS`, `CONNECTION_REQUIRED_PERIPHERALS`,
  `BATTERY_WARNING_REPEAT_MINUTES`.
- Update all uses in `led_feedback.c` as a pure rename (same behavior). `REQUIRED_PERIPHERALS`
  `depends on ZMK_SPLIT` and is available when connection or battery feedback is enabled.
- Add a `BUILD_ASSERT` for inactivity values 1-4 (`0` or 5-1440 only).
- Update `tests/firmware-build/config/lf_test_dongle.conf` if it uses renamed options.

Test: firmware build, config validation tests, and a manual build with inactivity values 1 and 4
(must fail) and 0 and 5 (must pass). Hardware: behavior identical to baseline.

### Stage 2: Shared warning engine (refactor, no new behavior)

- Introduce one warning engine in `led_feedback.c`: a single active warning at a time, a type
  (connection, no reading, low battery), a blink counter, a step, and the layer dismissal and
  reminder logic.
- Move the battery warning onto it first (it already has the N blink, dismiss and reminder
  logic), then the connection warning.
- Priority: connection, no reading, low battery. One active warning, no preemption, due warnings
  wait (see the spec's Warning priority section).
- Remove `warning_repeat_work` and its handler.
- Keep the blink look: connection shows the device number (same LED count each blink), battery
  blinks red then yellow, fixed 1 second pause.

Test: firmware build. Hardware: battery warning still blinks N times, layer change dismisses and
gives one reminder blink, no repeat.

### Stage 3: Connection warning on the shared engine

- The connection warning (after boot) uses `WARNING_BLINKS`, stops after N blinks, shows once per
  event, resolves immediately when the peripheral reconnects, and uses the layer dismiss and
  reminder blink.
- Event tracking: warned flag per type, reset when the condition goes false (and when all
  required peripherals are connected at the same time).
- Required check uses `slot_number()` and `REQUIRED_PERIPHERALS`, as today.

Test: firmware build. Hardware: disconnect a peripheral after boot and check one warning of
about 10 seconds, nothing after, a second disconnect gives a second warning, a layer change during
the warning gives one reminder blink.

### Stage 4: Boot warning

- At boot, if a required peripheral is missing and `ZMK_LED_FEEDBACK_CONNECTION` is on, run the
  continuous (no pause) boot warning for up to 30 seconds, or until all required peripherals are
  connected.
- It does not use up the "once per event" warning. A layer change interrupts it and it resumes
  after the layer indication with no reminder blink (the 30 seconds keep counting).
- At 30 seconds with a peripheral still missing: counts as warned, nothing more is shown.

Test: firmware build. Hardware: boot with a peripheral off (30 s then stop), turn it on within
30 s (stops early), layer change during the boot warning, then it resumes.

### Stage 5: Battery on required, connected peripherals

- Change `any_battery_low()` to only check required (`slot_number()` within the limit) and
  connected peripherals.
- Low battery: stops when that peripheral disconnects, new event after it reconnects and is low.

Test: firmware build. Hardware: required limit excludes a low peripheral (no warning),
disconnect a low peripheral (warning stops).

### Stage 6: No reading warning

- Add the yellow-only no reading warning: threshold above 0, a required, connected peripheral
  without a level. Evaluated 30 seconds after boot (right away after an inactivity reset).
- Priority above low battery. Not due in the first 30 seconds, so it blocks nothing before that.

Test: firmware build. Hardware: peripheral that never reports gives yellow blinks at 30 s,
stops when a reading arrives.

### Stage 7: Inactivity reset

- Own timer: record the time of the last key position event (any key press). Do not use ZMK
  idle or sleep state.
- When a key press comes after at least `WARNING_INACTIVITY_MINUTES` (counted from boot), start a
  new check period: stop the running warning, drop waiting ones, clear all warned state, run the
  boot warning if a peripheral is missing, then evaluate the other conditions as new events (no
  reading right away).
- No effect when the option is `0`.

Test: firmware build. Hardware (use a temporary short value in a local build only, the minimum
allowed is 5 minutes): leave the keyboard for the time, check no warning during inactivity, the
first key press starts the new period, a running warning is stopped by the reset.

### Stage 8: Docs and cleanup

- Update [docs/connection-feedback.md](connection-feedback.md),
  [docs/battery-feedback.md](battery-feedback.md), [docs/display-off.md](display-off.md), the
  README and the Kconfig help texts.
- Docs must cover: the new and removed options, the total warning length
  (blinks x (2 x `BLINK_INTERVAL_MS` + 1 s), about 10 seconds by default), the 30-second boot
  warning, the inactivity option and reset (and that it does nothing when connection feedback is
  off and `BATTERY_WARNING_LEVEL` is 0), and the "long warning delays higher priority warnings"
  trade-off with a high `WARNING_BLINKS`.
- Remove dead code and old option references (`grep` for removed option names in the whole repo).
- Final full pass: config validation tests, firmware build, and the whole hardware checklist
  (all scenarios in the spec).

## Risks

- **Layer interaction.** Layer dismissal, the reminder blink, display off and `OFF_OVERRIDES`
  all touch the same LED strip. Stage 2 is where regressions are most likely, so keep it a
  pure refactor and check the baseline checklist.
- **No automated runtime tests.** Behavior relies on hardware checks. If a native simulator
  setup is cheap to add, consider it after stage 2, but it is not required by this plan.
- **Event definition.** Tracking "condition false to true" per type needs care for the
  priority waiting case (a due warning that resolves while waiting is dropped).
- **Work queue timing.** Several delayable works (blink, boot window, inactivity) share the
  system work queue. Keep a single place that cancels all of them on reset.

## Decisions

- One branch (`warning-types`), one commit per stage.
- The inactivity value check (1-4 invalid) uses `BUILD_ASSERT`, like the existing numeric check.
  It is not covered by `tests/config-validation` (CMake only), so verify it once by hand with a
  firmware build using a value of 1 (must fail) and 5 (must pass).
