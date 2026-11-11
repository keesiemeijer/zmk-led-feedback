cmake_minimum_required(VERSION 3.20)
project(led_feedback_config_validation NONE)

enable_testing()

function(add_validation_case name expect_success)
    cmake_parse_arguments(PARSE_ARGV 2 CASE "" "LAYER;LAYER_COLORS;PER_LAYER_COLORS;BATTERY;BATTERY_COLORS;ERROR" "")

    add_test(
        NAME "${name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DMODULE_SOURCE_DIR=${CMAKE_CURRENT_LIST_DIR}/../.."
            "-DCASE_NAME=${name}"
            "-DEXPECT_SUCCESS=${expect_success}"
            "-DEXPECT_ERROR=${CASE_ERROR}"
            "-DLAYER=${CASE_LAYER}"
            "-DLAYER_COLORS=${CASE_LAYER_COLORS}"
            "-DPER_LAYER_COLORS=${CASE_PER_LAYER_COLORS}"
            "-DBATTERY=${CASE_BATTERY}"
            "-DBATTERY_COLORS=${CASE_BATTERY_COLORS}"
            "-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            -P "${CMAKE_CURRENT_LIST_DIR}/run_case.cmake"
    )
endfunction()

add_validation_case(layer_colors_are_valid ON LAYER ON
    LAYER_COLORS "00FF00,abcdef")
add_validation_case(layer_colors_with_invalid_hex_are_rejected OFF LAYER ON
    LAYER_COLORS "00FF0G"
    ERROR "CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS must be comma separated RRGGBB colors")
add_validation_case(empty_layer_colors_are_rejected OFF LAYER ON
    LAYER_COLORS ""
    ERROR "CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS must be comma separated RRGGBB colors")

add_validation_case(per_layer_colors_are_valid ON LAYER ON
    LAYER_COLORS "00FF00"
    PER_LAYER_COLORS "FF0000,00ff00")
add_validation_case(per_layer_colors_with_invalid_hex_are_rejected OFF LAYER ON
    LAYER_COLORS "00FF00"
    PER_LAYER_COLORS "FF0000,GGGGGG"
    ERROR "CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER must be")
add_validation_case(invalid_layer_colors_are_ignored_when_layer_feedback_is_disabled ON LAYER OFF
    LAYER_COLORS "not-a-color"
    PER_LAYER_COLORS "also-not-a-color")

add_validation_case(battery_colors_are_valid ON BATTERY ON
    BATTERY_COLORS "00FF00,FFFF00,FF0000,0000FF")
add_validation_case(battery_colors_with_wrong_count_are_rejected OFF BATTERY ON
    BATTERY_COLORS "00FF00,FFFF00,FF0000"
    ERROR "CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS must be")
add_validation_case(battery_colors_with_invalid_hex_are_rejected OFF BATTERY ON
    BATTERY_COLORS "00FF00,FFFF00,FF0000,00000G"
    ERROR "CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS must be")
add_validation_case(invalid_battery_colors_are_ignored_when_battery_feedback_is_disabled ON BATTERY OFF
    BATTERY_COLORS "not-a-color")

function(add_settings_case name expect_success option value expected_error)
    add_test(
        NAME "${name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DMODULE_SOURCE_DIR=${CMAKE_CURRENT_LIST_DIR}/../.."
            "-DCASE_NAME=${name}"
            "-DEXPECT_SUCCESS=${expect_success}"
            "-DEXPECT_ERROR=${expected_error}"
            "-DCHECK_SETTINGS=ON"
            "-DINPUT_NAME=CONFIG_${option}"
            "-DINPUT_VALUE=${value}"
            "-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            -P "${CMAKE_CURRENT_LIST_DIR}/run_case.cmake"
    )
endfunction()

add_settings_case(settings_defaults_are_valid ON "" "" "")
add_settings_case(required_none_is_valid ON ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS 0 "")
add_settings_case(required_all_is_valid ON ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS 2 "")
add_settings_case(required_too_many_is_rejected OFF ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS 3 "CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS")
add_settings_case(required_negative_is_rejected OFF ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS -2 "CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS")
add_settings_case(slot_order_permutation_is_valid ON ZMK_LED_FEEDBACK_SLOT_ORDER "2,1" "")
add_settings_case(slot_order_duplicate_is_rejected OFF ZMK_LED_FEEDBACK_SLOT_ORDER "1,1" "CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER")
add_settings_case(slot_order_short_is_rejected OFF ZMK_LED_FEEDBACK_SLOT_ORDER "1" "CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER")
add_settings_case(slot_order_extra_is_rejected OFF ZMK_LED_FEEDBACK_SLOT_ORDER "1,2,3" "CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER")
add_settings_case(slot_order_zero_is_rejected OFF ZMK_LED_FEEDBACK_SLOT_ORDER "0" "CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER")
add_settings_case(slot_order_malformed_is_rejected OFF ZMK_LED_FEEDBACK_SLOT_ORDER "2,x" "CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER")
add_settings_case(slot_order_out_of_range_is_rejected OFF ZMK_LED_FEEDBACK_SLOT_ORDER "1,3" "CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER")
add_settings_case(threshold_disabled_is_valid ON ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL 0 "")
add_settings_case(threshold_minimum_is_valid ON ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL 2 "")
add_settings_case(threshold_maximum_is_valid ON ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL 100 "")
add_settings_case(threshold_one_is_rejected OFF ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL 1 "CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL")
add_settings_case(threshold_negative_is_rejected OFF ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL -1 "CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL")
add_settings_case(threshold_above_maximum_is_rejected OFF ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL 101 "CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL")
add_settings_case(blink_interval_minimum_is_valid ON ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS 100 "")
add_settings_case(blink_interval_maximum_is_valid ON ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS 1000 "")
add_settings_case(blink_interval_too_short_is_rejected OFF ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS 99 "CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS")
add_settings_case(blink_interval_too_long_is_rejected OFF ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS 1001 "CONFIG_ZMK_LED_FEEDBACK_BLINK_INTERVAL_MS")
add_settings_case(blink_count_minimum_is_valid ON ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS 1 "")
add_settings_case(blink_count_maximum_is_valid ON ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS 10 "")
add_settings_case(blink_count_zero_is_rejected OFF ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS 0 "CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS")
add_settings_case(blink_count_too_many_is_rejected OFF ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS 11 "CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_BLINKS")
add_settings_case(brightness_minimum_zero_is_rejected OFF ZMK_LED_FEEDBACK_MIN_BRIGHTNESS 0 "CONFIG_ZMK_LED_FEEDBACK_MIN_BRIGHTNESS")
add_settings_case(brightness_maximum_256_is_rejected OFF ZMK_LED_FEEDBACK_MAX_BRIGHTNESS 256 "CONFIG_ZMK_LED_FEEDBACK_MAX_BRIGHTNESS")
add_settings_case(brightness_below_minimum_is_rejected OFF ZMK_LED_FEEDBACK_BRIGHTNESS 9 "CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS")
add_settings_case(brightness_above_maximum_is_rejected OFF ZMK_LED_FEEDBACK_BRIGHTNESS 129 "CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS")
add_settings_case(brightness_step_zero_is_rejected OFF ZMK_LED_FEEDBACK_BRIGHTNESS_STEP 0 "CONFIG_ZMK_LED_FEEDBACK_BRIGHTNESS_STEP")
add_settings_case(layer_list_valid ON ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON_LAYERS "0,2,7" "")
add_settings_case(layer_list_malformed_is_rejected OFF ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON_LAYERS "0, 2" "CONFIG_ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON_LAYERS")
add_settings_case(layer_list_false_word_is_rejected OFF ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON_LAYERS "OFF" "CONFIG_ZMK_LED_FEEDBACK_LAYER_ALWAYS_ON_LAYERS")
add_settings_case(layer_count_negative_is_rejected OFF ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT -2 "CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT")
add_settings_case(layer_count_above_255_is_valid_before_strip_check ON ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT 256 "")
add_settings_case(layer_count_above_maximum_is_rejected OFF ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT 65536 "CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER_LED_COUNT")
add_settings_case(layer_duration_zero_is_rejected OFF ZMK_LED_FEEDBACK_LAYER_DURATION_MS 0 "CONFIG_ZMK_LED_FEEDBACK_LAYER_DURATION_MS")
add_settings_case(battery_delay_zero_is_valid ON ZMK_LED_FEEDBACK_BATTERY_DELAY_MS 0 "")
add_settings_case(battery_duration_zero_is_rejected OFF ZMK_LED_FEEDBACK_BATTERY_DURATION_MS 0 "CONFIG_ZMK_LED_FEEDBACK_BATTERY_DURATION_MS")
