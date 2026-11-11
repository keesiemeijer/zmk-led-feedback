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
		late_connection_minimum
		late_second_connection
		delayed_deadline
		post_deadline_reconnects
		delayed_blink_work
		battery_restart_resolved
		battery_restart_unresolved
		display_cancellation
		display_pending
		display_pending_connection
		independent_options
		post_boot_updates
		maximum_battery_sequence)
	add_test(NAME "warning_state_${case}" COMMAND warning_state_test "${case}")
endforeach()