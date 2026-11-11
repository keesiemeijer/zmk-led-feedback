#!/bin/sh
set -eu

: "${ZMK_WORKSPACE:?Set ZMK_WORKSPACE to the West workspace}"
: "${MODULE_SOURCE_DIR:?Set MODULE_SOURCE_DIR to the module source}"
: "${ZMK_CONFIG_DIR:?Set ZMK_CONFIG_DIR to the firmware fixture configuration}"
: "${VALIDATION_BUILD_ROOT:?Set VALIDATION_BUILD_ROOT to a fresh output directory}"

mkdir -p "$VALIDATION_BUILD_ROOT"
cd "$ZMK_WORKSPACE"

run_case() {
	case_name=$1
	expected=$2
	mode=$3
	expected_message=$4
	shift 4
	build_dir="$VALIDATION_BUILD_ROOT/$case_name"
	log_file="$VALIDATION_BUILD_ROOT/$case_name.log"
	if [ -e "$build_dir" ] || [ -e "$log_file" ]; then
		printf '%s: output already exists; choose a fresh validation directory\n' "$case_name"
		return 1
	fi

	cmake_only=""
	if [ "$mode" = configure ]; then
		cmake_only="--cmake-only"
	fi
	if west build $cmake_only -s "$ZMK_WORKSPACE/zmk/app" -d "$build_dir" -b nice_nano_v2 -- \
		-DZephyr_DIR="$ZMK_WORKSPACE/zephyr/share/zephyr-package/cmake" \
		-DZMK_CONFIG="$ZMK_CONFIG_DIR" -DZMK_EXTRA_MODULES="$MODULE_SOURCE_DIR" \
		-DSHIELD=lf_test_dongle "$@" >"$log_file" 2>&1; then
		if [ "$expected" = failure ]; then
			printf '%s: unexpectedly succeeded\n' "$case_name"
			return 1
		fi
		printf '%s: accepted\n' "$case_name"
	else
		if [ "$expected" = success ] || ! grep -Fq -- "$expected_message" "$log_file"; then
			printf '%s: failed for an unexpected reason\n' "$case_name"
			tail -n 50 "$log_file"
			return 1
		fi
		printf '%s: rejected as expected\n' "$case_name"
	fi
}

run_case defaults success configure ""
grep -qx '# CONFIG_ZMK_LED_FEEDBACK_CONNECTION is not set' "$VALIDATION_BUILD_ROOT/defaults/zephyr/.config"
grep -qx '# CONFIG_ZMK_LED_FEEDBACK_BATTERY is not set' "$VALIDATION_BUILD_ROOT/defaults/zephyr/.config"
run_case connection_only success configure "" -DCONFIG_ZMK_LED_FEEDBACK_CONNECTION=y
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_CONNECTION=y' "$VALIDATION_BUILD_ROOT/connection_only/zephyr/.config"
grep -qx '# CONFIG_ZMK_LED_FEEDBACK_BATTERY is not set' "$VALIDATION_BUILD_ROOT/connection_only/zephyr/.config"
run_case battery_monitoring_only success configure "" -DCONFIG_ZMK_LED_FEEDBACK_BATTERY=y -DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=0
grep -qx '# CONFIG_ZMK_LED_FEEDBACK_CONNECTION is not set' "$VALIDATION_BUILD_ROOT/battery_monitoring_only/zephyr/.config"
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BATTERY=y' "$VALIDATION_BUILD_ROOT/battery_monitoring_only/zephyr/.config"
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=0' "$VALIDATION_BUILD_ROOT/battery_monitoring_only/zephyr/.config"
run_case battery_only success configure "" -DCONFIG_ZMK_LED_FEEDBACK_BATTERY=y -DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=15 -DCONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=1
grep -qx '# CONFIG_ZMK_LED_FEEDBACK_CONNECTION is not set' "$VALIDATION_BUILD_ROOT/battery_only/zephyr/.config"
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BATTERY=y' "$VALIDATION_BUILD_ROOT/battery_only/zephyr/.config"
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=15' "$VALIDATION_BUILD_ROOT/battery_only/zephyr/.config"
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=1' "$VALIDATION_BUILD_ROOT/battery_only/zephyr/.config"
run_case both_warnings success configure "" -DCONFIG_ZMK_LED_FEEDBACK_CONNECTION=y -DCONFIG_ZMK_LED_FEEDBACK_BATTERY=y -DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=15
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_CONNECTION=y' "$VALIDATION_BUILD_ROOT/both_warnings/zephyr/.config"
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BATTERY=y' "$VALIDATION_BUILD_ROOT/both_warnings/zephyr/.config"
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=15' "$VALIDATION_BUILD_ROOT/both_warnings/zephyr/.config"
run_case threshold_min success configure "" -DCONFIG_ZMK_LED_FEEDBACK_BATTERY=y -DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=2
run_case threshold_max success configure "" -DCONFIG_ZMK_LED_FEEDBACK_BATTERY=y -DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=100
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=2' "$VALIDATION_BUILD_ROOT/threshold_min/zephyr/.config"
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=100' "$VALIDATION_BUILD_ROOT/threshold_max/zephyr/.config"
run_case threshold_one failure configure 'must be 0 or 2-100' -DCONFIG_ZMK_LED_FEEDBACK_BATTERY=y -DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=1
run_case threshold_negative failure configure 'must be from 0 to 100' -DCONFIG_ZMK_LED_FEEDBACK_BATTERY=y -DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=-1
run_case threshold_too_high failure configure 'must be from 0 to 100' -DCONFIG_ZMK_LED_FEEDBACK_BATTERY=y -DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=101
run_case blink_interval_min success configure "" -DCONFIG_ZMK_LED_FEEDBACK_CONNECTION=y -DCONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=100
run_case blink_interval_max success configure "" -DCONFIG_ZMK_LED_FEEDBACK_CONNECTION=y -DCONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=1000
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=100' "$VALIDATION_BUILD_ROOT/blink_interval_min/zephyr/.config"
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=1000' "$VALIDATION_BUILD_ROOT/blink_interval_max/zephyr/.config"
run_case blink_interval_low failure configure 'must be from 100 to 1000' -DCONFIG_ZMK_LED_FEEDBACK_CONNECTION=y -DCONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=99
run_case blink_interval_high failure configure 'must be from 100 to 1000' -DCONFIG_ZMK_LED_FEEDBACK_CONNECTION=y -DCONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS=1001
run_case blink_count_min success configure "" -DCONFIG_ZMK_LED_FEEDBACK_BATTERY=y -DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=1
run_case blink_count_max success configure "" -DCONFIG_ZMK_LED_FEEDBACK_BATTERY=y -DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=10
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=1' "$VALIDATION_BUILD_ROOT/blink_count_min/zephyr/.config"
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=10' "$VALIDATION_BUILD_ROOT/blink_count_max/zephyr/.config"
run_case blink_count_low failure configure 'must be from 1 to 10' -DCONFIG_ZMK_LED_FEEDBACK_BATTERY=y -DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=0
run_case blink_count_high failure configure 'must be from 1 to 10' -DCONFIG_ZMK_LED_FEEDBACK_BATTERY=y -DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS=11
run_case required_zero success configure "" -DCONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=0
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=0' "$VALIDATION_BUILD_ROOT/required_zero/zephyr/.config"
run_case required_some success configure "" -DCONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=1
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=1' "$VALIDATION_BUILD_ROOT/required_some/zephyr/.config"
run_case required_all success configure "" -DCONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=2
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=2' "$VALIDATION_BUILD_ROOT/required_all/zephyr/.config"
run_case required_too_many failure configure 'must not exceed' -DCONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=3
run_case required_negative failure configure 'must be from -1 to 16' -DCONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=-2
run_case mapping_valid success configure "" '-DCONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="2,1"'
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="2,1"' "$VALIDATION_BUILD_ROOT/mapping_valid/zephyr/.config"
run_case mapping_identity success configure "" '-DCONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="1,2"'
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="1,2"' "$VALIDATION_BUILD_ROOT/mapping_identity/zephyr/.config"
run_case mapping_duplicate failure configure 'must be a permutation' '-DCONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="1,1"'
run_case mapping_short failure configure 'must have exactly 2 entries' '-DCONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="1"'
run_case mapping_zero failure configure 'must be a comma separated permutation' '-DCONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="0"'
run_case mapping_out_of_range failure configure 'must be a permutation' '-DCONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="1,3"'
run_case mapping_malformed failure configure 'comma separated permutation' '-DCONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="2,x"'
run_case brightness_min success configure "" -DCONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS=10
run_case brightness_max success configure "" -DCONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS=128
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS=10' "$VALIDATION_BUILD_ROOT/brightness_min/zephyr/.config"
grep -qx 'CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS=128' "$VALIDATION_BUILD_ROOT/brightness_max/zephyr/.config"
run_case brightness_order failure configure 'must be between' -DCONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS=9
run_case brightness_high failure configure 'must be from 1 to 255' -DCONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS=256
run_case strip_count_all success build "" -DCONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT=-1
run_case strip_count_cycling success build "" -DCONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT=0
run_case strip_count_max success build "" -DCONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT=4
run_case strip_count_too_high failure build 'CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT exceeds strip length' -DCONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT=5
run_case removed_display_off failure configure 'undefined symbol ZMK_LED_FEEDBACK_DISPLAY_OFF' -DCONFIG_ZMK_LED_FEEDBACK_DISPLAY_OFF=y
run_case removed_override failure configure 'undefined symbol ZMK_LED_FEEDBACK_OFF_OVERRIDES' '-DCONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES="battery_warning"'
run_case removed_repeat failure configure 'undefined symbol ZMK_LED_FEEDBACK_BATTERY_WARNING_REPEAT_MINUTES' -DCONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_REPEAT_MINUTES=5
run_case removed_required_name failure configure 'undefined symbol ZMK_LED_FEEDBACK_CONNECTION_REQUIRED_PERIPHERALS' -DCONFIG_ZMK_LED_FEEDBACK_CONNECTION_REQUIRED_PERIPHERALS=1

printf 'Real ZMK configuration validation passed\n'
