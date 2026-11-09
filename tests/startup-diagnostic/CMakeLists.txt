cmake_minimum_required(VERSION 3.20)
project(startup_diagnostic_test C)

enable_testing()
find_package(Threads REQUIRED)

add_executable(startup_diagnostic_test
    startup_diagnostic_test.c
    ../../src/startup_diagnostic.c
)
target_include_directories(startup_diagnostic_test PRIVATE
    include
    ../../src
)
add_test(NAME startup_diagnostic_window COMMAND startup_diagnostic_test)

function(add_handoff_test name warning_level overrides)
    add_executable(${name} handoff_test.c ../../src/startup_diagnostic.c)
    target_include_directories(${name} PRIVATE mock ../../src ../../include)
    target_link_libraries(${name} PRIVATE Threads::Threads)
    target_compile_definitions(${name} PRIVATE
        CONFIG_ZMK_LED_FEEDBACK_BATTERY_WARNING_LEVEL=${warning_level}
        CONFIG_ZMK_LED_FEEDBACK_OFF_OVERRIDES="${overrides}")
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_handoff_test(handoff_warning 15 "")
add_handoff_test(handoff_warning_override 15 "battery_warning")
add_handoff_test(handoff_no_warning 0 "")

add_executable(diagnostic_disabled_test disabled_test.c)
target_include_directories(diagnostic_disabled_test PRIVATE mock ../../src ../../include)
target_compile_definitions(diagnostic_disabled_test PRIVATE CONFIG_ZMK_LED_FEEDBACK_STARTUP_DIAGNOSTIC=0)
target_link_libraries(diagnostic_disabled_test PRIVATE Threads::Threads)
add_test(NAME diagnostic_disabled COMMAND diagnostic_disabled_test)
