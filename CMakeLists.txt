zephyr_include_directories(include)
target_sources(app PRIVATE src/behavior_led_feedback.c src/behavior_led_feedback_battery.c)
target_sources_ifdef(CONFIG_ZMK_LED_FEEDBACK app PRIVATE src/led_feedback.c)
target_sources_ifdef(CONFIG_ZMK_LED_FEEDBACK app PRIVATE src/warning_state.c)

set(_lf_hex "[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]")

if(CONFIG_ZMK_LED_FEEDBACK_LAYER AND NOT CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS MATCHES "^(${_lf_hex},)*${_lf_hex}$")
  message(FATAL_ERROR "CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS must be comma separated RRGGBB colors, got \"${CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS}\"")
endif()

if(CONFIG_ZMK_LED_FEEDBACK_LAYER AND CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER)
  if(NOT CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER MATCHES "^(${_lf_hex},)*${_lf_hex}$")
    message(FATAL_ERROR "CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER must be comma separated RRGGBB colors, got \"${CONFIG_ZMK_LED_FEEDBACK_LAYER_COLORS_PER_LAYER}\"")
  endif()
endif()

if(CONFIG_ZMK_LED_FEEDBACK_BATTERY AND NOT CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS MATCHES "^${_lf_hex},${_lf_hex},${_lf_hex},${_lf_hex}$")
  message(FATAL_ERROR "CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS must be 4 comma separated RRGGBB colors, got \"${CONFIG_ZMK_LED_FEEDBACK_BATTERY_COLORS}\"")
endif()

if(CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES AND NOT CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES MATCHES "^(layer_feedback|layer_change|battery_feedback|battery_warning|connection_warning)(,(layer_feedback|layer_change|battery_feedback|battery_warning|connection_warning))*$")
  message(FATAL_ERROR "CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES must be a comma separated list of layer_feedback, layer_change, battery_feedback, battery_warning, and connection_warning, got \"${CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES}\"")
endif()
