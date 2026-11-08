cmake_minimum_required(VERSION 3.20)
project(led_feedback_config_fixture LANGUAGES C)

function(zephyr_include_directories)
endfunction()

function(target_sources_ifdef condition target)
    if(${condition})
        target_sources(${target} ${ARGN})
    endif()
endfunction()

add_library(app STATIC dummy.c)

set(CONFIG_ZMK_LED_FEEDBACK OFF)
set(CONFIG_ZMK_LED_FEEDBACK_LAYER "${LAYER}")
set(CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS "${LAYER_COLORS}")
set(CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER "${PER_LAYER_COLORS}")
set(CONFIG_ZMK_LED_FEEDBACK_BATTERY "${BATTERY}")
set(CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS "${BATTERY_COLORS}")
set(CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES "${OVERRIDES}")

add_subdirectory("${MODULE_SOURCE_DIR}" led_feedback)
