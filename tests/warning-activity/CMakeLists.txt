cmake_minimum_required(VERSION 3.13)
project(warning_activity C)

enable_testing()
add_executable(warning_activity_test warning_activity_test.c)
add_test(NAME warning_activity COMMAND warning_activity_test)
