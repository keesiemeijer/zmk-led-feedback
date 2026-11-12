# hardware test plan

## Status

Run these checks after the relevant implementation and automated tests are complete. They are a plan, not results from tests already performed. The [specification](spec.md) controls expected behavior.

The configurations use the current option names. All seven dongle profiles and the awake, sleep,
and no-report peripheral configurations were built against ZMK v0.3; every explicitly requested
setting matched the generated `.config`. Tests 3-5 have passed their reported visual checks on
the revised-cutoff firmware. Exact event timestamps were not captured and are not required for
the visual hardware checks. Exact cutoff and event-order cases are covered by automated state and
runtime tests; no near-deadline hardware timing runs are required. The earlier Test 2 observation
used the previous firmware and does not verify the revised cutoff. Recheck effective settings for
each build.

## Hardware and preparation

- One `nice_nano_v2`-compatible dongle with a four-LED WS2812 strip, plus the two battery-powered `keyworks_lily58` halves. The existing dongle overlay uses SPI3, P0.06, and GRB color mapping. Confirm the actual board, LED wiring, power, and logic-level requirements before flashing or powering the strip. Use the correct board if it is not compatible with `nice_nano_v2`.
- The existing Lily58 shield, layout, keymap, `mouse.dtsi`, and layer hooks from the `lily58` workspace. The LED key bindings must still be present after implementation; if bindings or behavior names change, update this plan and the complete test keymap before testing.
- A USB data connection to the dongle, a way to turn each half on and off, a stopwatch or video recording, and a way to observe each half's reported percentage. For cases requiring a low report, at least one required peripheral must actually report a nonzero level below the selected threshold. Do not short, overdischarge, or otherwise force a battery to an unsafe voltage.
- Preserve the existing pairing order and working configuration. Use an isolated copy of the complete Lily58 `config` directory and the complete profiles below; do not overwrite the user's live configuration. Normal firmware updates do not require resetting pairings. Only reset settings and re-pair if explicitly intended, since that erases saved pairings.
- This plan assumes the left half is device 1 and the right half is device 2, using the identity slot mapping. Verify actual pairing slots first; if they differ, adjust the device names in the procedure or explicitly re-pair using the existing pairing guide. Do not assume a powered-on half has already connected or reported battery status.
- The current Lily58 keymap uses layer 3 for LED controls. From the default layer, hold the left thumb key at position 52 (`&mo 4`) and tap the left Shift-position key at position 36 (`&to 3`), then release. In layer 3, the left-side keys include `LF_ON` at position 13, brightness up at 14, `&led_bat 1` at 15, `&led_bat 2` at 16, `LF_OFF` at 25, brightness down at 26, `&led_bat 0` at 28, continuous-layer toggle at 37, and display toggle at 38. Return to layer 0 with `&to 0` at position 12. Confirm the actual layout and bindings before use.

## Complete configurations

All configurations below are complete contents for the named `.conf` files, not patches to an existing profile. They use the unchanged Lily58 keymap and shield files described above. Keep any additional files in the isolated `config` directory, including `west.yml` and `mouse.dtsi`; do not combine multiple dongle profiles. The two configured slots and `-1` default require both peripherals except in Profile E.

### Peripheral configuration: awake

Use this complete file as both `config/keyworks_lily58_left.conf` and `config/keyworks_lily58_right.conf` for Profiles A-E. Build the halves as split peripherals, as shown below.

```ini
CONFIG_ZMK_SPLIT=y
CONFIG_ZMK_SPLIT_ROLE_CENTRAL=n
CONFIG_EC11=y
CONFIG_EC11_TRIGGER_GLOBAL_THREAD=y
CONFIG_ZMK_POINTING=y
CONFIG_ZMK_DISPLAY=y
CONFIG_ZMK_WIDGET_BATTERY_STATUS_SHOW_PERCENTAGE=y
CONFIG_ZMK_BATTERY_REPORTING=y
CONFIG_ZMK_SLEEP=n
```

### Peripheral configuration: sleep test

For the sleep/wake timing case only, use this complete file as both peripheral `.conf` files, rebuild and flash both halves, then restore the complete awake configuration and reflash them before other tests. `45000` is milliseconds.

```ini
CONFIG_ZMK_SPLIT=y
CONFIG_ZMK_SPLIT_ROLE_CENTRAL=n
CONFIG_EC11=y
CONFIG_EC11_TRIGGER_GLOBAL_THREAD=y
CONFIG_ZMK_POINTING=y
CONFIG_ZMK_DISPLAY=y
CONFIG_ZMK_WIDGET_BATTERY_STATUS_SHOW_PERCENTAGE=y
CONFIG_ZMK_BATTERY_REPORTING=y
CONFIG_ZMK_SLEEP=y
CONFIG_ZMK_IDLE_TIMEOUT=30000
CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=45000
```

### Peripheral configuration: no battery report

For the connected-but-not-reporting case only, use this complete file for device 2's peripheral `.conf` file. Keep device 1 on the complete awake configuration, and rebuild and flash device 2. Verify the effective configuration disables both battery reporting and the Bluetooth Battery Service. Restore and reflash the complete awake configuration after the case.

```ini
CONFIG_ZMK_SPLIT=y
CONFIG_ZMK_SPLIT_ROLE_CENTRAL=n
CONFIG_EC11=y
CONFIG_EC11_TRIGGER_GLOBAL_THREAD=y
CONFIG_ZMK_POINTING=y
CONFIG_ZMK_DISPLAY=n
CONFIG_ZMK_BATTERY_REPORTING=n
CONFIG_BT_BAS=n
CONFIG_ZMK_SLEEP=n
```

### Profile A: default warnings off

Complete `config/keyworks_lily58_dongle.conf`. The warning options are intentionally omitted to test their defaults; battery feedback is enabled for on-demand checks.

```ini
CONFIG_ZMK_SPLIT=y
CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y
CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2
CONFIG_ZMK_POINTING=y
CONFIG_ZMK_DISPLAY=n
CONFIG_SPI=y
CONFIG_LED_STRIP=y
CONFIG_WS2812_STRIP=y
CONFIG_ZMK_SLEEP=n
CONFIG_ZMK_LED_FEEDBACK=y
CONFIG_ZMK_LED_FEEDBACK_LAYER=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS=64
CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS=10
CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS=128
CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS="800080,00FFFF,00FFFF"
CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="1,2"
```

### Profile B: connection warnings only

Complete `config/keyworks_lily58_dongle.conf`. Battery monitoring remains available on demand, but the warning threshold is disabled.

```ini
CONFIG_ZMK_SPLIT=y
CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y
CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2
CONFIG_ZMK_POINTING=y
CONFIG_ZMK_DISPLAY=n
CONFIG_SPI=y
CONFIG_LED_STRIP=y
CONFIG_WS2812_STRIP=y
CONFIG_ZMK_SLEEP=n
CONFIG_ZMK_LED_FEEDBACK=y
CONFIG_ZMK_LED_FEEDBACK_LAYER=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
CONFIG_ZMK_LED_FEEDBACK_CONNECTION=y
CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=-1
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=0
CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=500
CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS=64
CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS=10
CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS=128
CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS="800080,00FFFF,00FFFF"
CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="1,2"
```

### Profile C: both warnings

Complete `config/keyworks_lily58_dongle.conf`. A threshold of 100 flags a valid report from 1% through 99%, not 100%. This is a test threshold, not a recommended everyday setting.

```ini
CONFIG_ZMK_SPLIT=y
CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y
CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2
CONFIG_ZMK_POINTING=y
CONFIG_ZMK_DISPLAY=n
CONFIG_SPI=y
CONFIG_LED_STRIP=y
CONFIG_WS2812_STRIP=y
CONFIG_ZMK_SLEEP=n
CONFIG_ZMK_LED_FEEDBACK=y
CONFIG_ZMK_LED_FEEDBACK_LAYER=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
CONFIG_ZMK_LED_FEEDBACK_CONNECTION=y
CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=-1
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=100
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=5
CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=500
CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS=64
CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS=10
CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS=128
CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS="800080,00FFFF,00FFFF"
CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="1,2"
```

### Profile D: battery warnings only

Complete `config/keyworks_lily58_dongle.conf`. The required count is explicitly set to all configured slots, even though connection warnings are disabled.

```ini
CONFIG_ZMK_SPLIT=y
CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y
CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2
CONFIG_ZMK_POINTING=y
CONFIG_ZMK_DISPLAY=n
CONFIG_SPI=y
CONFIG_LED_STRIP=y
CONFIG_WS2812_STRIP=y
CONFIG_ZMK_SLEEP=n
CONFIG_ZMK_LED_FEEDBACK=y
CONFIG_ZMK_LED_FEEDBACK_LAYER=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
CONFIG_ZMK_LED_FEEDBACK_CONNECTION=n
CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=-1
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=100
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=5
CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=500
CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS=64
CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS=10
CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS=128
CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS="800080,00FFFF,00FFFF"
CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="1,2"
```

### Profile E: one required peripheral

Complete `config/keyworks_lily58_dongle.conf`. With the assumed pairing and identity mapping, device 1 is required and device 2 is optional. Verify the shared setting filters both connection and battery warnings.

```ini
CONFIG_ZMK_SPLIT=y
CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y
CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2
CONFIG_ZMK_POINTING=y
CONFIG_ZMK_DISPLAY=n
CONFIG_SPI=y
CONFIG_LED_STRIP=y
CONFIG_WS2812_STRIP=y
CONFIG_ZMK_SLEEP=n
CONFIG_ZMK_LED_FEEDBACK=y
CONFIG_ZMK_LED_FEEDBACK_LAYER=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
CONFIG_ZMK_LED_FEEDBACK_CONNECTION=y
CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=1
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=100
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=5
CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=500
CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS=64
CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS=10
CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS=128
CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS="800080,00FFFF,00FFFF"
CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="1,2"
```

### Profile F: trace-enabled both warnings

Complete `config/keyworks_lily58_dongle.conf` for timing measurements only. USB logging alone does not provide the required module events: before using this profile, add temporary test instrumentation or an implementation-provided diagnostic facility that records module boot start, processed connection changes, processed battery reports including slot and percentage, warning transitions, and boot closure using monotonic uptime. Confirm the logs actually contain those events. Do not retain diagnostic changes in production firmware unless intentionally designed and documented.

```ini
CONFIG_ZMK_SPLIT=y
CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y
CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2
CONFIG_ZMK_POINTING=y
CONFIG_ZMK_DISPLAY=n
CONFIG_SPI=y
CONFIG_LED_STRIP=y
CONFIG_WS2812_STRIP=y
CONFIG_ZMK_SLEEP=n
CONFIG_ZMK_USB_LOGGING=y
CONFIG_LOG_PROCESS_THREAD_STARTUP_DELAY_MS=8000
CONFIG_ZMK_LED_FEEDBACK=y
CONFIG_ZMK_LED_FEEDBACK_LAYER=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
CONFIG_ZMK_LED_FEEDBACK_CONNECTION=y
CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=-1
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=100
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=5
CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=500
CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS=64
CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS=10
CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS=128
CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS="800080,00FFFF,00FFFF"
CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="1,2"
```

### Profile G: trace-enabled one required peripheral

Complete `config/keyworks_lily58_dongle.conf` for the optional-device battery-filter timing case. As with Profile F, USB logging must be accompanied by verified temporary or implementation-provided event instrumentation.

```ini
CONFIG_ZMK_SPLIT=y
CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y
CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2
CONFIG_ZMK_POINTING=y
CONFIG_ZMK_DISPLAY=n
CONFIG_SPI=y
CONFIG_LED_STRIP=y
CONFIG_WS2812_STRIP=y
CONFIG_ZMK_SLEEP=n
CONFIG_ZMK_USB_LOGGING=y
CONFIG_LOG_PROCESS_THREAD_STARTUP_DELAY_MS=8000
CONFIG_ZMK_LED_FEEDBACK=y
CONFIG_ZMK_LED_FEEDBACK_LAYER=y
CONFIG_ZMK_LED_FEEDBACK_BATTERY=y
CONFIG_ZMK_LED_FEEDBACK_CONNECTION=y
CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=1
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=100
CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=5
CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=500
CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS=64
CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS=10
CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS=128
CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS="800080,00FFFF,00FFFF"
CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="1,2"
```

## Build and flash procedure

Use the existing ZMK v0.3 West workspace and local Docker image described in the [Stage 0 baseline](implementation-strategy.md#stage-0-baseline). Copy the complete Lily58 `config` directory to a new isolated test directory, use the complete peripheral configurations above, and choose one complete dongle profile. The workspace's active West manifest must still use ZMK v0.3. The `lily58` module supplies the real shield, layout, and hooks; this module must be mounted from the branch under test, not fetched from the Lily58 manifest's `warning-types` revision.

In the following commands, `TEST_DIR` is the absolute path to the isolated test directory containing `config/`. Create a fresh output directory for each profile; do not reuse an existing build directory or delete a user's existing build. Set `PROFILE` to `a`, `b`, `c`, `d`, `e`, `f`, or `g` to label the output. These commands require the same macOS workspace layout and Docker image as Stage 0:

```sh
export TEST_DIR="/absolute/path/to/isolated-test-directory"
export PROFILE="a"
docker run --rm --entrypoint sh \
  -v "$HOME/github/zmk-build-workspace:/workspace:ro" \
  -v "$HOME/github/zmk-led-feedback:/module:ro" \
  -v "$HOME/github/lily58:/keyboard:ro" \
  -v "$TEST_DIR:/test" -w /workspace \
  zmkfirmware/zmk-dev-arm:4.1-branch -c '
    west build -s /workspace/zmk/app -d /test/build/dongle-'"$PROFILE"' -b nice_nano_v2 -- \
      -DZephyr_DIR=/workspace/zephyr/share/zephyr-package/cmake \
      -DZMK_CONFIG=/test/config "-DZMK_EXTRA_MODULES=/module;/keyboard" \
      -DSHIELD=keyworks_lily58_dongle
  '
```

For trace Profiles F and G, use the following complete dongle build command instead. The `zmk-usb-logging` snippet supplies the required CDC ACM devicetree overlay; setting `CONFIG_ZMK_USB_LOGGING` alone is not sufficient with the existing dongle overlay. Set `PROFILE` to `f` or `g` and use the matching complete configuration above.

```sh
export TEST_DIR="/absolute/path/to/isolated-test-directory"
export PROFILE="f"
docker run --rm --entrypoint sh \
  -v "$HOME/github/zmk-build-workspace:/workspace:ro" \
  -v "$HOME/github/zmk-led-feedback:/module:ro" \
  -v "$HOME/github/lily58:/keyboard:ro" \
  -v "$TEST_DIR:/test" -w /workspace \
  zmkfirmware/zmk-dev-arm:4.1-branch -c '
    west build -s /workspace/zmk/app -d /test/build/dongle-'"$PROFILE"' -b nice_nano_v2 -S zmk-usb-logging -- \
      -DZephyr_DIR=/workspace/zephyr/share/zephyr-package/cmake \
      -DZMK_CONFIG=/test/config "-DZMK_EXTRA_MODULES=/module;/keyboard" \
      -DSHIELD=keyworks_lily58_dongle
  '
```

Build the two peripheral images from the complete selected peripheral configuration. Use new output directory names again when switching between awake and sleep configurations:

```sh
docker run --rm --entrypoint sh \
  -v "$HOME/github/zmk-build-workspace:/workspace:ro" \
  -v "$HOME/github/zmk-led-feedback:/module:ro" \
  -v "$HOME/github/lily58:/keyboard:ro" \
  -v "$TEST_DIR:/test" -w /workspace \
  zmkfirmware/zmk-dev-arm:4.1-branch -c '
    west build -s /workspace/zmk/app -d /test/build/left-awake -b nice_nano_v2 -- \
      -DZephyr_DIR=/workspace/zephyr/share/zephyr-package/cmake \
      -DZMK_CONFIG=/test/config "-DZMK_EXTRA_MODULES=/module;/keyboard" \
      -DSHIELD=keyworks_lily58_left \
      -DCONFIG_ZMK_SPLIT=y -DCONFIG_ZMK_SPLIT_ROLE_CENTRAL=n
  '
docker run --rm --entrypoint sh \
  -v "$HOME/github/zmk-build-workspace:/workspace:ro" \
  -v "$HOME/github/zmk-led-feedback:/module:ro" \
  -v "$HOME/github/lily58:/keyboard:ro" \
  -v "$TEST_DIR:/test" -w /workspace \
  zmkfirmware/zmk-dev-arm:4.1-branch -c '
    west build -s /workspace/zmk/app -d /test/build/right-awake -b nice_nano_v2 -- \
      -DZephyr_DIR=/workspace/zephyr/share/zephyr-package/cmake \
      -DZMK_CONFIG=/test/config "-DZMK_EXTRA_MODULES=/module;/keyboard" \
      -DSHIELD=keyworks_lily58_right \
      -DCONFIG_ZMK_SPLIT=y -DCONFIG_ZMK_SPLIT_ROLE_CENTRAL=n
  '
```

For the sleep profile, use the complete sleep peripheral configuration and replace the two output names with unused `left-sleep` and `right-sleep` directories. For the no-report case, use the complete no-report configuration for device 2 and a fresh `right-no-report` build directory. Verify the resulting `.config` files, especially module enabled, role, two slots, warning enables, threshold, required count, and battery reporting. For Profile A, confirm the warning settings are actually at their disabled defaults. An ignored Kconfig assignment, unknown option, or warning indicating an invalid requested value makes the profile invalid; update it and rebuild before testing. Record known unrelated deprecation warnings separately.

Flash the resulting `build/<output>/zephyr/zmk.uf2` to the corresponding device using its normal bootloader procedure. Flash each half only when its peripheral configuration changes; flash the dongle when switching dongle profiles. Allow the devices to reconnect using existing pairings. Do not use `settings_reset` as part of routine profile changes. Restore the user's normal firmware and settings after testing.

## Observation and timing rules

- Use observation from dongle power-on only for coarse visual timing. Do not try to judge whether an event occurred just before or after the 20-second cutoff, or whether one blink fits exactly. Those boundary cases are covered by automated tests; exact hardware traces are optional diagnostics, not a pass requirement.
- The full default battery warning is five red/yellow blinks, 500 ms per color, separated by four one-second dark pauses: approximately nine seconds, no trailing pause. Connection red blinks use 500 ms on and 500 ms off. In normal operation a connected peripheral can take time to report its battery after connecting.
- Profile F has the same warning settings as Profile C plus USB logging; Profile G similarly corresponds to Profile E. USB logging does not itself provide module event traces and can affect timing. Use the non-trace profiles for ordinary visual checks; Profiles F/G and instrumentation are optional diagnostics only.
- Before any low-battery test, note each half's actual reported percentage. If no required device reports `1-99`, the 100% threshold cannot trigger a low warning. Mark that test blocked or inconclusive rather than claiming success. A zero or missing reading is unavailable, not a low report.
- With the current four-LED strip and identity mapping, one or two blinking red LEDs indicate the highest-numbered missing required device. Existing Lily58 hooks customize layer colors; do not mistake the orange or purple layer indications for warnings. If fewer LEDs or another mapping are used, use the corresponding generic warning rules instead.
- Run each power-on scenario more than once. Record profile, firmware build, pairing slots, reported battery levels, actions, observed LED patterns, and pass/fail/inconclusive. Do not perform 19-20-second timing trials; use the deterministic state tests for exact boundary and queue-order assertions.

## Hardware tests

### 1. Defaults and ordinary controls

**Configuration:** Profile A and the complete awake peripheral configurations. Confirm both warning options have their disabled defaults in the built dongle `.config`.

1. Connect the dongle and both halves. Power-cycle the dongle and observe for at least 30 seconds; change layers and use the key bindings from the preparation section.
2. Show each battery with `&led_bat 1`, `&led_bat 2`, and `&led_bat 0`. Switch the display off and back on, and test brightness up and down.
3. Power-cycle the dongle with one or both halves off. Observe for at least 30 seconds. When possible, reconnect the left half and request the missing right half's battery after boot.ss

**Expected:** no automatic connection or battery warnings, even if a required device is missing or a battery would be below the test threshold. Ordinary layer feedback, battery requests, display controls, and brightness previews work when the display is on. A missing or zero battery reading shows one no-reading LED, blue with the default battery palette. Turning the display off keeps the strip dark until a display-on or brightness control turns it back on.

Passed with expected behaviour.

### 2. Connection warning priority

**Configuration:** Profile B and the complete awake peripheral configurations.

1. Leave device 1 on and device 2 off, then power-cycle the dongle. Observe the initial connection and the rest of the 20-second boot window.
2. Once device 1 has connected, use its keys to switch layers and request layer feedback while device 2 remains off. Test `LF_OFF` and then `LF_ON` or a brightness key while the connection warning is still active.
3. In a separate run, power device 2 on during boot. In another run, leave it off until well after boot, then power it on. For a coarse no-new-warning check, power-cycle with both devices connected, wait at least 30 seconds from dongle power-on, then turn device 2 off and observe for five seconds. A trace-confirmed run can instead use the recorded module deadline and processed-disconnection timestamp.

**Expected:** the connection warning starts during the boot period while required devices are missing and updates as they connect. With only device 2 missing, two red LEDs are lit during a red phase. Layer changes and requests do not replace a rendered warning, although the keyboard changes layers normally. Display off stops all LEDs; turning it back on while the warning remains active restores the warning instead of a preview. Connection output does not continue after boot. There is no battery warning in this profile. Exact phase-fit behavior is covered by automated tests, not this visual check.

**Recorded result:** passed with expected behavior for the tested scenario. The run used the previous firmware and does not verify the revised cutoff. The revised exact cutoff is covered by automated tests; no near-deadline hardware retest is required.

### 3. Low battery after unresolved connection

**Configuration:** Profile C and the complete awake peripheral configurations. Keep device 1 on and device 2 off. Device 1 must report a nonzero level below 100% during boot. No trace or timestamp instrumentation is needed for this visual check.

1. Note device 1's reported battery percentage. Power on device 1, leave device 2 off, then power-cycle the dongle and record the LEDs through boot and afterward.
2. Observe the connection warning while device 2 is missing. After the boot period, connection output should stop; if device 1 reports low during boot, the battery sequence should then run.
3. Wait until all LEDs finish and confirm the battery sequence does not repeat without another boot.

**Expected:** rendered connection red phases take priority during boot. After the boot period, connection output stops and the full five-cycle red/yellow battery sequence starts if device 1 reports low during boot, even though device 2 remains missing. It is not repeated. A missing battery report from device 2 alone never produces a battery warning. If the battery sequence does not appear, mark the visual check inconclusive; do not infer module event timing from the battery percentage shown on the half.

**Recorded result (revised-cutoff firmware):** visual pass reported. The connection warning appeared, followed after boot by one full battery warning sequence that did not repeat. Exact module-processed event timestamps were not captured, so the trace-only timing detail remains unverified.

### 4. Low battery after connection resolves

**Configuration:** Profile C and the complete awake peripheral configurations. Device 1 must report `1-99` during boot. No trace or timestamp instrumentation is needed for this visual check.

1. Start with device 1 on and device 2 off. Power-cycle the dongle.
2. While the connection warning is visible, turn on device 2 and allow it to connect.
3. Observe through the end of the battery sequence and for at least ten seconds afterward.

**Expected:** the red connection warning stops when both required devices connect, then one complete five-blink red/yellow battery warning starts. No automatic battery warning repeats. If the battery warning does not appear, mark the visual check inconclusive rather than guessing when a report was processed.

**Recorded result (revised-cutoff firmware):** passed with expected visible behavior. Exact processed-event timestamps were not captured.

### 5. Battery-only opt-in and interrupted warning

**Configuration:** Profile D for steps 1-2, then use complete Profile C for step 3. Use the complete awake peripheral configurations throughout. A required peripheral must report `1-99`.

1. With Profile D, power-cycle the dongle with both halves on. Observe through boot and until any battery warning finishes. Repeat with device 2 left off.
2. Check on-demand battery display after boot, without rebooting:
3. Wait until the boot period has ended and any battery warning already in progress has finished.
4. Request each peripheral's battery separately with `&led_bat 1` and `&led_bat 2`.
5. Observe each result. If device 2 is off, its reading may be unavailable; repeat with both halves connected to confirm both readings display.
6. With Profile C and both halves on, power-cycle the dongle. While a battery sequence is active, turn one required half off. In a separate run, reconnect it promptly while the red connection warning is visible; observe whether the battery warning starts over after connection output ends. Do not time the action against the boot cutoff.

**Expected:** Profile D shows a full battery warning on a qualifying report but no connection warning, even when device 2 is missing. Once the battery warning completes, further low reports do not repeat it, and on-demand battery displays work after boot. In step 3, a visible disconnection warning takes priority over an active battery sequence; after connection output ends, the battery sequence restarts from its first red phase. If the warning or restart is not visible, mark that run inconclusive. Exact interruption and deadline ordering are covered by automated tests.

**Recorded result:** visual checks passed. Profile D showed the battery warning without a connection warning, including with device 2 off; after the boot window, an on-demand request for the unavailable device showed one blue LED. With Profile F, the battery warning completed after the boot-period connection warning, and reconnecting a peripheral before the deadline produced one full battery warning with no repeat. Exact processed-event timestamps and ordering were not captured in a trace.

### 6. Coarse post-boot exclusion check

**Configuration:** Profile B and the complete awake peripheral configurations.

1. Power-cycle the dongle with both halves connected and let the boot period finish.
2. Turn device 2 off after at least 30 seconds from dongle power-on and observe for five seconds. No new connection warning should start.

**Expected:** the warning does not start after the boot period. This is a coarse visual confirmation only; exact cutoff, full-phase fit, queued-event, and scheduling behavior are verified by automated tests, not by manual timing.

**Recorded result:** passed. No connection warning appeared after 30 seconds.

### 7. Display-off cancellation of a battery warning

**Configuration:** Profile C and the complete awake peripheral configurations. Both halves must connect and a required peripheral must report `1-99` for a battery sequence to begin during boot.

1. Enter layer 3 and, while the red/yellow sequence is active, press `LF_OFF` using a connected half.
2. Turn the display back on after the sequence has been canceled and observe without rebooting. Do not attempt to time a display-on action against the boot cutoff.

**Expected:** `LF_OFF` immediately makes the strip dark and cancels an already-started battery warning. That sequence does not resume or replay when turned on. Pending-warning behavior at the boot cutoff is covered by automated state tests.

**Recorded result:** passed. Turning the display off canceled the active battery warning; turning it back on without rebooting did not replay the warning.

### 8. Required versus optional peripherals

**Configuration:** Profile E and the complete awake peripheral configurations. Device 1 must be the required left half; device 2 is optional.

1. Start device 1 and leave device 2 off. Power-cycle the dongle and wait through boot.
2. Start device 2 after boot and use the on-demand battery keys for both devices.
3. Optional: use on-demand requests to show device 1 at 100% and device 2 at `1-99`. Exact optional-device battery filtering during boot is covered by automated tests; without event instrumentation, do not treat an absent warning alone as proof that both reports arrived in time.

**Expected:** any initial connection indication clears once required device 1 connects; missing optional device 2 does not keep it active. A low report from required device 1 may still produce a battery warning. Device 2 remains available to on-demand battery display when connected. Optional-device battery filtering is asserted by automated tests.

**Recorded result:** steps 1-2 passed. With device 1 reporting low and device 2 off, a battery warning appeared; no further warnings appeared during the rest of boot or afterward. After reconnecting device 2, on-demand battery requests displayed both peripherals' readings. Optional step 3 was not tested.

### 9. Optional trace diagnostics

This test is optional and is not required for hardware-test completion. Skip it unless verified module event instrumentation is available. USB logging alone is insufficient.

If performed, use Profile F/G with instrumentation that records monotonic module boot, processed connection and battery events, warning transitions, and boot closure. Do not infer exact event timing from a stopwatch, the battery percentage shown on a half, or USB logging without those events. Automated state/runtime tests remain the authoritative timing checks.

### 10. Connected peripheral without battery reporting

**Configuration:** Profile D, the complete awake configuration for device 1, and the complete no-battery-report configuration for device 2. For the repeat after the initial inconclusive run, use the test-specific dongle build with battery warning threshold 80; all other Profile D settings are unchanged. Confirm device 1 remains at or above 80% during boot so it cannot independently trigger a warning.

1. Build and flash device 2 with the no-report configuration and confirm its effective `.config` disables both battery reporting and Battery Service. Power on both halves and power-cycle the dongle.
2. Confirm device 2 connects and can send a key press. Observe the strip through the boot window, then use device 1 to request `&led_bat 2`.
3. Restore the complete awake configuration and reflash device 2 after this case.

**Expected:** a connected device that does not report battery status causes no automatic battery warning; an on-demand request for that device shows one no-reading LED. If device 2 cannot connect or its effective configuration still provides a battery reading, the hardware test is inconclusive; do not treat a disconnected peripheral as proof of the connected/no-report case. The deterministic event tests remain required.

**Recorded result:** passed on the threshold-80 repeat. Device 1 was reported at 84%, above the test threshold; no automatic battery warning appeared. Device 2 connected, sent key presses, and its on-demand battery request showed one blue no-reading LED. The initial threshold-100 run was inconclusive because device 1's 84% report triggered a battery warning.

## Automated-only and inconclusive cases

Use the fake-clock tests for exact-at-deadline ordering, delayed queued events, simultaneous connection/battery events, already-started warning cancellation while interrupted, multiple rapid reconnects, and all validation boundaries. Use firmware configuration/build tests for rejected invalid settings, all warning-enable combinations, and strips shorter than the number of peripheral slots. Hardware can supplement those cases but does not replace deterministic assertions.

Record missing hardware, incompatible profiles, unavailable low-battery readings, or unobserved expected behavior as unverified. Lack of USB trace logging does not invalidate a visual hardware check; exact timing and event-order claims remain covered by automated tests. Restore normal dongle and peripheral firmware after the tests.
