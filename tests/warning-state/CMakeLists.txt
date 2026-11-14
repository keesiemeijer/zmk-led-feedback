cmake_minimum_required(VERSION 3.20)
project(led_feedback_warning_state C)

enable_testing()

add_executable(warning_state_test warning_state_test.c ../../src/warning_state.c)
target_include_directories(warning_state_test PRIVATE ../../src)
set_target_properties(warning_state_test PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(warning_state_test PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

foreach(case IN ITEMS
	invalid_internal_config
		disabled_defaults
		required_connections
		battery_eligibility
		battery_sequence
		connection_priority
		unresolved_connection
		boot_cutoff
		connection_render_deadline_boundaries
		suppressed_connection_preserves_battery
		delayed_deadline
		post_deadline_reconnects
		delayed_blink_work
		battery_restart_resolved
		battery_restart_unresolved
		repeated_battery_interruptions
		battery_latch_healthy_and_zero
		display_cancellation
		display_pending
		display_pending_connection
		independent_options
		post_boot_updates
		maximum_battery_sequence)
	add_test(NAME "warning_state_${case}" COMMAND warning_state_test "${case}")
endforeach()

set(fake_include_dir "${CMAKE_CURRENT_BINARY_DIR}/fake-include")
foreach(header IN ITEMS
		zephyr/bluetooth/bluetooth.h
		zephyr/bluetooth/conn.h
		zephyr/device.h
		zephyr/drivers/led_strip.h
		zephyr/init.h
		zephyr/kernel.h
		zmk/ble.h
		zmk/event_manager.h
		zmk/events/battery_state_changed.h
		zmk/events/layer_state_changed.h
		zmk/events/position_state_changed.h
		zmk/keymap.h)
	get_filename_component(header_dir "${fake_include_dir}/${header}" DIRECTORY)
	file(MAKE_DIRECTORY "${header_dir}")
	file(WRITE "${fake_include_dir}/${header}" "#include <fake_zephyr.h>\n")
endforeach()

add_executable(warning_runtime_test ../warning-runtime/runtime_test.c ../warning-runtime/test_hooks.c ../../src/warning_state.c)
target_include_directories(warning_runtime_test PRIVATE ../warning-runtime "${fake_include_dir}" ../../include ../../src)
set_target_properties(warning_runtime_test PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(warning_runtime_test PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-unused-function)
endif()
foreach(case IN ITEMS priority_and_restore display_controls controls_during_battery_warning
	brightness_stops_at_bounds early_connection battery_only_feedback
	interrupted_battery_display battery_restart delayed_deadline display_off_before_init layer_hooks
	layer_inactivity_timeout layer_timeout_waits_for_battery_warning
	layer_timeout_display_off)
	add_test(NAME "warning_runtime_${case}" COMMAND warning_runtime_test "${case}")
endforeach()
foreach(case IN ITEMS connection_phase_exact_deadline delayed_connection_render
	suppressed_connection_preserves_battery)
	add_test(NAME "warning_runtime_${case}"
		COMMAND warning_runtime_test "${case}")
endforeach()

add_executable(warning_runtime_battery_only_test ../warning-runtime/runtime_test.c ../warning-runtime/test_hooks.c ../../src/warning_state.c)
target_include_directories(warning_runtime_battery_only_test PRIVATE ../warning-runtime "${fake_include_dir}" ../../include ../../src)
target_compile_definitions(warning_runtime_battery_only_test PRIVATE
	CONFIG_ZMK_LED_FEEDBACK_CONNECTION=0 CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=1)
set_target_properties(warning_runtime_battery_only_test PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(warning_runtime_battery_only_test PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-unused-function)
endif()
add_test(NAME warning_runtime_battery_only_required_filter
	COMMAND warning_runtime_battery_only_test battery_only_required_filter)

add_executable(warning_runtime_mapped_test ../warning-runtime/runtime_test.c ../warning-runtime/test_hooks.c ../../src/warning_state.c)
target_include_directories(warning_runtime_mapped_test PRIVATE ../warning-runtime "${fake_include_dir}" ../../include ../../src)
target_compile_definitions(warning_runtime_mapped_test PRIVATE
	CONFIG_ZMK_LED_FEEDBACK_REQUIRED_PERIPHERALS=1 CONFIG_ZMK_LED_FEEDBACK_SLOT_ORDER="2,1")
set_target_properties(warning_runtime_mapped_test PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(warning_runtime_mapped_test PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-unused-function)
endif()
add_test(NAME warning_runtime_mapped_required_filter
	COMMAND warning_runtime_mapped_test mapped_required_filter)