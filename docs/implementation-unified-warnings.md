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

## Hardware checks

Automated tests can't check the blinking, so each stage ends with these checks on the dongle. Put
the config in the dongle's `.conf` file, build the firmware from the `warning-types` branch, flash
the dongle and reboot it. Every config below is complete: replace the whole config, don't merge.

Terms:

- A is peripheral 1 and B is peripheral 2 (device numbers from `SLOT_ORDER`).
- "Battery low" means `BATTERY_WARNING_LEVEL` is above the percentage A reports. Read A's level
  with the battery key first. Example: A reports 70, so use `80`. Replace `80` in every config
  below with a value above A's level.
- "Layer key" is any key that changes the layer.
- "Wait for the boot" means wait 40 seconds after the dongle starts and A and B are connected.

### Base config

```ini
CONFIG_ZMK_LED_FEEDBACK_LAYER=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=80
```

Defaults: connection feedback on, 5 blinks, all peripherals required, blink interval 500 ms.

### Stage 0 and 1: baseline and option rename

Result: all steps were run with the `warning-types` branch after stage 1, not on `main`. The
notes below are the baseline for the later stages. The `main` configs fail to build on this branch
(the old option names no longer exist), which is expected. Stage 1 only removed the battery repeat,
which was off by default.

Config for steps 1 to 4: base config.

1. Turn on A and B and wait for the boot. Reboot the dongle. The battery warning shows 5 blinks
   (red then yellow, 1 second pause), then nothing. Wait 10 minutes: no second warning.
    - I didn't see a second warning, but i did not have my eyes on the dongle the entire 10minutes
2. Reboot the dongle and press the layer key during the battery warning. The layer indication
   shows, then one reminder blink, then nothing.
    - Expected results
3. With both on and the battery warning finished, turn off B. Write down what the dongle show and
   for how long. Press the layer key during it and write down what happens. Turn B on: it stops.
    - 2 blinking leds (without a pause between them) continously. Doesn't stop blinking.
      the layer keys also don't stop it. Turning it back on stops it.
4. Turn off B while the battery warning is running. Write down which warning shows first.
    - First after the battery warning is the 2 blinking leds witout a pause between them
5. Flash the config below and reboot the dongle: the battery warning is 3 blinks.

    Stage 1 config:

    ```ini
    CONFIG_ZMK_LED_FEEDBACK_LAYER=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=80
    CONFIG_ZMK_LED_FEEDBACK_WARNING_BLINKS=3
    ```

- Two red LEDs for a very short time (the connection warning at boot), then 3 battery blinks.

    `main` config (the option has the old name):

    ```ini
    CONFIG_ZMK_LED_FEEDBACK_LAYER=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=80
    CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=3
    ```

- Expected on this branch: build warning, the old symbol is undefined.

6. Flash the config below. Turn off B: no connection warning. Turn B on and turn off A:
   connection warning for A.

- Expected result

    Stage 1 config:

    ```ini
    CONFIG_ZMK_LED_FEEDBACK_LAYER=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=80
    CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=1
    ```

    `main` config (the option has the old name):

    ```ini
    CONFIG_ZMK_LED_FEEDBACK_LAYER=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=80
    CONFIG_ZMK_LED_FEEDBACK_CONNECTION_REQUIRED_PERIPHERALS=1
    ```

- Expected on this branch: build warning, the old symbol is undefined.

### Stage 2: shared engine (no new behavior)

Config: base config.

1. Repeat steps 1 to 4 of stage 0 and 1. The results must match your notes, except step 4 (the
   spec's priority rules apply: connection first, a running warning is never interrupted).
    - All expected results.

Known interim differences (the connection warning is still continuous until stage 3):

- A battery warning waits while the connection warning runs, so with B off it only shows after
  B reconnects.
    - Yes. Expected results
- Warnings wait until a layer indication has finished, instead of taking over the display.
    - Checked with a 10 second layer duration and the layer key held: the warning started about
      10 seconds after the press. Expected results.
- The connection check is event driven instead of polling every 500 ms.

### Stage 3: connection warning on the engine

Config for steps 1 to 6 (battery warning off, 3 blinks):

```ini
CONFIG_ZMK_LED_FEEDBACK_LAYER=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=0
CONFIG_ZMK_LED_FEEDBACK_WARNING_BLINKS=3
```

1. Turn on A and B and wait for the boot. Turn off B.
2. The connection warning shows 3 blinks with 1 second pauses, then stops. Wait 2 minutes: nothing
   more is shown while B is off.
3. Turn B on, wait for it to connect, turn it off again: a new 3 blink warning.
4. During a warning press the layer key: the layer indication shows, then one reminder blink, then
   nothing.
5. After a warning has finished press the layer key: no reminder blink.
6. Turn off B, and turn it on again after the first blink: the warning stops right away.

### Stage 4: boot warning

Config for steps 1 to 4: same as stage 3.

1. Turn off B and reboot the dongle. The connection warning blinks continuously (no pauses). After
   30 seconds it stops and nothing more is shown.
2. After that turn B on, nothing shows. Turn it off: a regular 3 blink warning.
3. Turn off B, reboot the dongle and turn B on after 10 seconds: the blinking stops when B
   connects.
4. Turn off B, reboot and press the layer key after 10 seconds: the layer indication shows, then
   the continuous blinking resumes without a reminder blink, and stops at 30 seconds after boot.
5. Flash the config below (connection feedback off), reboot with B off: nothing blinks.

    ```ini
    CONFIG_ZMK_LED_FEEDBACK_LAYER=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=0
    CONFIG_ZMK_LED_FEEDBACK_WARNING_BLINKS=3
    CONFIG_ZMK_LED_FEEDBACK_CONNECTION=n
    ```

### Stage 5: battery on required, connected peripherals

Config for steps 1 and 2:

```ini
CONFIG_ZMK_LED_FEEDBACK_LAYER=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=80
CONFIG_ZMK_LED_FEEDBACK_WARNING_BLINKS=3
CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=1
```

1. A low, A and B connected: one low battery warning shows.
2. With A low and connected, turn off A: the low battery warning stops right away and the
   connection warning for A shows. Turn A on: the low battery warning shows again.
3. Flash the config below (replace `70` with a value between the battery levels of A and B, so only
   B is low). Only B is low and B is not required: no warning.

    ```ini
    CONFIG_ZMK_LED_FEEDBACK_LAYER=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=70
    CONFIG_ZMK_LED_FEEDBACK_WARNING_BLINKS=3
    CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=1
    ```

### Stage 6: no reading warning

You need a peripheral that doesn't report a battery level (the battery key shows the "no reading"
color). Use a peripheral without battery reporting for this.

Config for steps 1 to 3:

```ini
CONFIG_ZMK_LED_FEEDBACK_LAYER=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=80
CONFIG_ZMK_LED_FEEDBACK_WARNING_BLINKS=3
```

1. Reboot with both connected. Nothing shows in the first 30 seconds. At about 30 seconds 3 yellow
   blinks show.
2. With A low and the other peripheral without a reading: the low battery warning shows first,
   then the yellow warning follows after it, once 30 seconds have passed.
3. When a reading arrives the yellow warning stops right away.

### Stage 7: inactivity reset

5 is the minimum inactivity value, use it in a local build only.

Config for steps 1 to 5:

```ini
CONFIG_ZMK_LED_FEEDBACK_LAYER=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=80
CONFIG_ZMK_LED_FEEDBACK_WARNING_BLINKS=3
CONFIG_ZMK_LED_FEEDBACK_WARNING_INACTIVITY_MINUTES=5
```

1. Turn on A and B and wait for the boot. Turn off B and let the 3 blink warning finish.
2. Don't press any key for 5 minutes. Nothing blinks during that time.
3. Press a key: the continuous boot warning shows (up to 30 seconds) because B is still off. Turn
   B on: it stops.
4. Press a key within 5 minutes after any warning: no new warning (no new period).
5. Repeat 1 and 2 with A low and its warning finished: the first key press gives the boot warning
   (if B is off), then a new low battery warning.
6. Flash the config below (long warning). Turn off B and wait for the boot warning to end. Press
   no keys for 5 minutes, then press one while the warning is still blinking. The running warning
   stops and a new period starts.

    ```ini
    CONFIG_ZMK_LED_FEEDBACK_LAYER=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=80
    CONFIG_ZMK_LED_FEEDBACK_WARNING_BLINKS=100
    CONFIG_ZMK_LED_FEEDBACK_WARNING_INACTIVITY_MINUTES=5
    ```

7. Flash the config below (inactivity reset off). Repeat 1 and 2, then press a key: nothing shows.

    ```ini
    CONFIG_ZMK_LED_FEEDBACK_LAYER=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
    CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=80
    CONFIG_ZMK_LED_FEEDBACK_WARNING_BLINKS=3
    CONFIG_ZMK_LED_FEEDBACK_WARNING_INACTIVITY_MINUTES=0
    ```

### Stage 8: final pass

Config: base config (all defaults: 5 blinks, no inactivity reset). Repeat the checks of stages 3
to 5, using their configs with the `WARNING_BLINKS` line removed. Check the defaults: 5 blinks,
about 10 seconds per warning, no inactivity reset.

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
