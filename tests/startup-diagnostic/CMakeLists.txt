cmake_minimum_required(VERSION 3.20)
project(startup_diagnostic_test C)

enable_testing()

add_executable(startup_diagnostic_test
    startup_diagnostic_test.c
    ../../src/startup_diagnostic.c
)
target_include_directories(startup_diagnostic_test PRIVATE
    include
    ../../src
)
add_test(NAME startup_diagnostic_window COMMAND startup_diagnostic_test)
