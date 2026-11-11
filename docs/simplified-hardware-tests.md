# Simplified hardware test plan

## Status

Run these checks after the relevant implementation and automated tests are complete. They are a plan, not results from tests already performed. The [simplified specification](simplified-spec.md) controls expected behavior.

The configurations use the current option names. All seven dongle profiles and the awake, sleep,
and no-report peripheral configurations were built against ZMK v0.3; every explicitly requested
setting matched the generated `.config`. These builds were not flashed or tested on hardware.
Recheck the effective configuration for your own builds; do not silently drop an option or assume
a successful build means the requested value was applied.

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

Use the existing ZMK v0.3 West workspace and local Docker image described in the [Stage 0 baseline](simplified-implementation-strategy.md#stage-0-baseline). Copy the complete Lily58 `config` directory to a new isolated test directory, use the complete peripheral configurations above, and choose one complete dongle profile. The workspace's active West manifest must still use ZMK v0.3. The `lily58` module supplies the real shield, layout, and hooks; this module must be mounted from the branch under test, not fetched from the Lily58 manifest's `warning-types` revision.

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

- Use a video or stopwatch from dongle power-on for approximate visual timing. It does not establish exact time since module initialization or the instant ZMK processed a BLE event. For exact cutoff claims, use Profile F's verified trace and compare timestamps to its recorded boot-start time; allow normal scheduling and logging delays.
- The full default battery warning is five red/yellow blinks, 500 ms per color, separated by four one-second dark pauses: approximately nine seconds, no trailing pause. Connection red blinks use 500 ms on and 500 ms off. In normal operation a connected peripheral can take time to report its battery after connecting.
- Profile F has the same warning settings as Profile C plus USB logging; Profile G similarly corresponds to Profile E. When a procedure requires trace evidence, use the complete trace profile and snippet-based build command, not an undocumented change to the non-trace configuration. The profiles delay log processing for eight seconds to help capture early boot messages, but capture is not guaranteed: verify that startup events are present and no required messages were dropped. Logging can affect timing, so also observe representative runs with the matching non-trace profile.
- Before any low-battery test, note each half's actual reported percentage. If no required device reports `1-99`, the 100% threshold cannot trigger a low warning. Mark that test blocked or inconclusive rather than claiming success. A zero or missing reading is unavailable, not a low report.
- With the current four-LED strip and identity mapping, one or two blinking red LEDs indicate the highest-numbered missing required device. Existing Lily58 hooks customize layer colors; do not mistake the orange or purple layer indications for warnings. If fewer LEDs or another mapping are used, use the corresponding generic warning rules instead.
- Run each power-on scenario more than once. Record profile, firmware build, pairing slots, reported battery levels, actions, trace/video timestamps, observed LED patterns, and pass/fail/inconclusive. Test-specific timing at 19-20 seconds is not a pass unless the relevant _processed_ event is confirmed inside the window; use the deterministic state tests for exact boundary and queue-order assertions.

## Hardware tests

### 1. Defaults and ordinary controls

**Configuration:** Profile A and the complete awake peripheral configurations. Confirm both warning options have their disabled defaults in the built dongle `.config`.

1. Connect the dongle and both halves. Power-cycle the dongle and observe for at least 30 seconds; change layers and use the key bindings from the preparation section.
2. Show each battery with `&led_bat 1`, `&led_bat 2`, and `&led_bat 0`. Switch the display off and back on, and test brightness up and down.
3. Power-cycle the dongle with one or both halves off. Observe for at least 30 seconds. When possible, reconnect the left half and request the missing right half's battery after boot.

**Expected:** no automatic connection or battery warnings, even if a required device is missing or a battery would be below the test threshold. Ordinary layer feedback, battery requests, display controls, and brightness previews work when the display is on. A missing or zero battery reading shows one no-reading LED, blue with the default battery palette. Turning the display off keeps the strip dark until a display-on or brightness control turns it back on.

### 2. Connection warning priority

**Configuration:** Profile B and the complete awake peripheral configurations.

1. Leave device 1 on and device 2 off, then power-cycle the dongle. Observe the initial connection and the rest of the 20-second boot window.
2. Once device 1 has connected, use its keys to switch layers and request layer feedback while device 2 remains off. Test `LF_OFF` and, before the deadline, `LF_ON` or a brightness key.
3. In a separate run, power device 2 on during boot. In another run, leave it off until after boot and then power it on. After a boot with both devices connected, turn device 2 off after the deadline.

**Expected:** the connection warning starts during the 20-second boot window while required devices are missing and updates as they connect. With only device 2 missing, two red LEDs blink. Layer changes and requests do not replace the warning, although the keyboard changes layers normally. Display off stops all LEDs; turning it on before the deadline shows any still-eligible connection warning instead of a preview. The warning stops as soon as both required devices connect or, if still missing, after satisfying the two-blink minimum. A disconnection processed at or after the deadline does not start or extend it. There is no battery warning in this profile.

### 3. Low battery after unresolved connection

**Configuration:** Profile F and the complete awake peripheral configurations. Device 1 must report `1-99` during boot; use the verified trace for the report timing. Repeat a representative run with complete Profile C to observe behavior without USB tracing.

1. Power on device 1 and leave device 2 off. Power-cycle the dongle and record the full boot and subsequent LEDs.
2. Confirm from the trace that device 1's nonzero low report was processed before the deadline while device 2 remained missing.
3. Wait until all LEDs finish, then observe beyond the end of boot without power-cycling.

**Expected:** connection red blinks take priority for the boot window; once the connection warning ends, the full five-blink red/yellow battery sequence runs, even though device 2 remains missing. It is not repeated. A missing battery report from device 2 alone never produces a battery warning. If the low report cannot be confirmed before boot closes, the battery portion is inconclusive.

### 4. Low battery after connection resolves

**Configuration:** Profile F and the complete awake peripheral configurations. Device 1 must report `1-99` during boot; verify both the battery report and connection timing in the trace.

1. Start with device 1 on and device 2 off. Power-cycle the dongle.
2. After device 1's low report is processed, turn on device 2 early enough for its connection event to be processed before the deadline.
3. Observe through the end of the battery sequence and for at least ten seconds afterward.

**Expected:** the red connection warning stops when both required devices connect, then one complete five-blink red/yellow battery warning starts without waiting for the boot deadline. No automatic battery warning repeats. If the connection or low report is only processed after the deadline, record the case as inconclusive for this scenario.

### 5. Battery-only opt-in and interrupted warning

**Configuration:** Profile D for steps 1-2, then rebuild and flash the dongle using complete Profile F for step 3. Use the complete awake peripheral configurations throughout. A required peripheral must report `1-99`.

1. With Profile D, power-cycle the dongle with both halves on. Observe as the first low report is processed. Repeat with device 2 left off.
2. Wait until after boot and request each battery level; then observe without rebooting.
3. With Profile F and both halves on, power-cycle the dongle. After the connection warning clears and the battery sequence starts, turn one required half off before that sequence finishes and before boot ends. In separate runs, reconnect it before the deadline and leave it off through the deadline. Confirm the processed event ordering in the trace.

**Expected:** Profile D shows a full battery warning on a qualifying report but no connection warning, even when device 2 is missing. Once the battery warning completes, further low reports do not repeat it, and on-demand battery displays work after boot. In step 3, a connection warning interrupts a battery warning only if the disconnection is processed before the deadline; when the connection warning ends, the battery warning starts again from its first red phase and completes in full. Partial battery blinks may repeat. If the disconnect is processed too late or the battery sequence has already completed, retry and mark the attempted interruption inconclusive.

### 6. Late connection and post-deadline exclusion

**Configuration:** Profile B and the complete awake peripheral configurations. Use Profile F only when exact trace timestamps are needed; it enables battery warnings, so use peripherals reporting 100% or no valid low reading for an unambiguous connection-only observation.

1. Start with both halves connected, then power-cycle the dongle. During boot, turn device 2 off close to 20 seconds, leaving device 1 connected. Confirm by trace that the disconnection was processed before the deadline and that fewer than two red blinks had completed by the deadline.
2. Keep device 2 off long enough to finish the minimum. In separate runs, reconnect it during the continuation, or turn the display off with device 1 during the continuation.
3. In a separate trace-confirmed run, start the late warning for device 1 and turn device 2 off only after boot closes. Reconnect device 1 while device 2 remains off.

**Expected:** the pre-deadline connection warning begins immediately and, if still unresolved, completes at least two red blinks even when that extends beyond boot. Reconnecting all eligible devices or turning the display off stops it immediately. A device whose disconnection is processed after the deadline does not change or extend the continuing indication; once all devices eligible at the deadline reconnect, the warning stops even if a later device remains disconnected. Exact scheduling boundaries and a reconnect-then-disconnect race are also covered by deterministic tests.

### 7. Display-off cancellation of a battery warning

**Configuration:** Profile F and the complete awake peripheral configurations. Both halves must connect and a required peripheral must report `1-99` early enough to begin the battery sequence during boot.

1. Enter layer 3 as soon as the halves connect. While the red/yellow sequence is active, press `LF_OFF` using a connected half.
2. Turn the display back on before boot ends. Repeat in another run, leaving it off until after boot. Separately start with device 1 reporting 100% and device 2 off, turn the display off with device 1, then power device 2 on so its `1-99` report is processed before boot closes and before any battery sequence has started.

**Expected:** `LF_OFF` immediately makes the strip dark and cancels an already-started battery warning. That sequence does not resume or replay when turned on, before or after boot. A qualifying report received while off can produce its still-unstarted warning if the display is turned on before the deadline; if still off at the deadline, it is discarded. If the input cannot be delivered or the timing cannot be observed, mark the relevant case inconclusive and use the automated state tests.

### 8. Required versus optional peripherals

**Configuration:** Profile E and the complete awake peripheral configurations for steps 1-2; rebuild and flash the dongle with complete Profile G for step 3. Device 1 must be the required left half; device 2 is optional.

1. Start device 1 and leave device 2 off. Power-cycle the dongle and wait through boot.
2. Start device 2 after boot and use the on-demand battery keys for both devices.
3. For the battery-filter case, first verify device 1 reports exactly 100% and device 2 reports `1-99`. Power-cycle with both on and confirm both reports were processed during boot.

**Expected:** any initial connection indication clears once required device 1 connects; missing optional device 2 does not keep it active. A low report from required device 1 may still produce a battery warning in steps 1-2. Device 2 remains available to on-demand battery display when connected. In step 3, the optional device's low report does not trigger a battery warning, while device 1's 100% report is not below the test threshold. If the required battery is not 100%, the optional battery case is inconclusive because it could itself trigger the aggregate warning.

### 9. Connection and battery timing measurements

**Configuration:** Profile F and the complete awake peripheral configurations. For the sleep/wake run, rebuild and reflash both halves using the complete sleep peripheral configuration, then restore the awake configuration afterward.

1. Confirm the trace contains monotonic timestamps for boot start, processed peripheral connections and battery reports, warning transitions, and boot closure. Connect to the dongle's USB logging interface; if unavailable or events are absent, do not claim exact timing results.
2. Run multiple cold dongle starts with both halves already awake, then with halves powered on one at a time after the dongle. Record every processed connection and first valid report relative to module initialization, including missing and late reports.
3. With the sleep peripheral firmware, leave both halves idle long enough to enter sleep, power-cycle the dongle, wake each half with a key press, and record the same events. Repeat with staggered wake-ups and normal reconnections.
4. Compare the trace with video of the LEDs. Repeat representative checks with the non-trace Profile C because USB logging and instrumentation can affect timing.

**Expected:** the measured boot window is 20 seconds from module initialization, subject to normal work-scheduling delay. Reports first processed at or after the deadline do not create a warning. A missing or delayed reading never produces a battery warning. Record the observed distribution and all missed reports; do not claim that 20 seconds guarantees successful connection or reporting on all hardware.

### 10. Connected peripheral without battery reporting

**Configuration:** Profile D, the complete awake configuration for device 1, and the complete no-battery-report configuration for device 2. Device 1 must report 100% or have no valid low reading during boot, so it cannot independently trigger a battery warning.

1. Build and flash device 2 with the no-report configuration and confirm its effective `.config` disables both battery reporting and Battery Service. Power on both halves and power-cycle the dongle.
2. Confirm device 2 connects and can send a key press. Observe the strip through the boot window, then use device 1 to request `&led_bat 2`.
3. Restore the complete awake configuration and reflash device 2 after this case.

**Expected:** a connected device that does not report battery status causes no automatic battery warning; an on-demand request for that device shows one no-reading LED. If device 2 cannot connect or its effective configuration still provides a battery reading, the hardware test is inconclusive; do not treat a disconnected peripheral as proof of the connected/no-report case. The deterministic event tests remain required.

## Automated-only and inconclusive cases

Use the fake-clock tests for exact-at-deadline ordering, delayed queued events, simultaneous connection/battery events, already-started warning cancellation while interrupted, multiple rapid reconnects, and all validation boundaries. Use firmware configuration/build tests for rejected invalid settings, all warning-enable combinations, and strips shorter than the number of peripheral slots. Hardware can supplement those cases but does not replace deterministic assertions.

Record any missing hardware, unavailable trace facility, incompatible profile, 100%/unknown battery report, missed 20-second window, or unavailable USB logging as unverified. Do not turn an inconclusive observation into a pass. Restore normal dongle and peripheral firmware after the tests.
