cmake_minimum_required(VERSION 3.20)
project(led_feedback_config_validation NONE)

enable_testing()

function(add_validation_case name expect_success overrides)
    cmake_parse_arguments(PARSE_ARGV 3 CASE "" "LAYER;LAYER_COLORS;PER_LAYER_COLORS;BATTERY;BATTERY_COLORS;ERROR" "")

    add_test(
        NAME "${name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DMODULE_SOURCE_DIR=${CMAKE_CURRENT_LIST_DIR}/../.."
            "-DCASE_NAME=${name}"
            "-DEXPECT_SUCCESS=${expect_success}"
            "-DEXPECT_ERROR=${CASE_ERROR}"
            "-DOVERRIDES=${overrides}"
            "-DLAYER=${CASE_LAYER}"
            "-DLAYER_COLORS=${CASE_LAYER_COLORS}"
            "-DPER_LAYER_COLORS=${CASE_PER_LAYER_COLORS}"
            "-DBATTERY=${CASE_BATTERY}"
            "-DBATTERY_COLORS=${CASE_BATTERY_COLORS}"
            "-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            -P "${CMAKE_CURRENT_LIST_DIR}/run_case.cmake"
    )
endfunction()

add_validation_case(empty_list_is_valid ON "")
add_validation_case(layer_feedback_is_valid ON "layer_feedback")
add_validation_case(layer_change_is_valid ON "layer_change")
add_validation_case(battery_feedback_is_valid ON "battery_feedback")
add_validation_case(battery_warning_is_valid ON "battery_warning")
add_validation_case(connection_warning_is_valid ON "connection_warning")
add_validation_case(combined_list_is_valid ON "battery_warning,connection_warning")
add_validation_case(all_overrides_are_valid ON "layer_feedback,layer_change,battery_feedback,battery_warning,connection_warning")
add_validation_case(unknown_item_is_invalid OFF "unknown_feedback"
    ERROR "CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES must be a comma separated list")
add_validation_case(spaced_items_are_invalid OFF "layer_feedback, connection_warning"
    ERROR "CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES must be a comma separated list")
add_validation_case(trailing_comma_is_invalid OFF "layer_feedback,"
    ERROR "CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES must be a comma separated list")
add_validation_case(empty_item_is_invalid OFF "layer_feedback,,battery_warning"
    ERROR "CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES must be a comma separated list")

add_validation_case(layer_colors_are_valid ON "" LAYER ON
    LAYER_COLORS "00FF00,abcdef")
add_validation_case(layer_colors_with_invalid_hex_are_rejected OFF "" LAYER ON
    LAYER_COLORS "00FF0G"
    ERROR "CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS must be comma separated RRGGBB colors")
add_validation_case(empty_layer_colors_are_rejected OFF "" LAYER ON
    LAYER_COLORS ""
    ERROR "CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS must be comma separated RRGGBB colors")

add_validation_case(per_layer_colors_are_valid ON "" LAYER ON
    LAYER_COLORS "00FF00"
    PER_LAYER_COLORS "FF0000,00ff00")
add_validation_case(per_layer_colors_with_invalid_hex_are_rejected OFF "" LAYER ON
    LAYER_COLORS "00FF00"
    PER_LAYER_COLORS "FF0000,GGGGGG"
    ERROR "CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER must be")
add_validation_case(invalid_layer_colors_are_ignored_when_layer_feedback_is_disabled ON "" LAYER OFF
    LAYER_COLORS "not-a-color"
    PER_LAYER_COLORS "also-not-a-color")

add_validation_case(battery_colors_are_valid ON "" BATTERY ON
    BATTERY_COLORS "00FF00,FFFF00,FF0000,0000FF")
add_validation_case(battery_colors_with_wrong_count_are_rejected OFF "" BATTERY ON
    BATTERY_COLORS "00FF00,FFFF00,FF0000"
    ERROR "CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS must be")
add_validation_case(battery_colors_with_invalid_hex_are_rejected OFF "" BATTERY ON
    BATTERY_COLORS "00FF00,FFFF00,FF0000,00000G"
    ERROR "CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS must be")
add_validation_case(invalid_battery_colors_are_ignored_when_battery_feedback_is_disabled ON "" BATTERY OFF
    BATTERY_COLORS "not-a-color")
