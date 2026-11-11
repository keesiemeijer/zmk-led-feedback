set(case_binary_dir "${BINARY_DIR}/${CASE_NAME}")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${CMAKE_CURRENT_LIST_DIR}/fixture"
        -B "${case_binary_dir}"
        "-DMODULE_SOURCE_DIR=${MODULE_SOURCE_DIR}"
        "-DLAYER=${LAYER}"
        "-DLAYER_COLORS=${LAYER_COLORS}"
        "-DPER_LAYER_COLORS=${PER_LAYER_COLORS}"
        "-DBATTERY=${BATTERY}"
        "-DBATTERY_COLORS=${BATTERY_COLORS}"
        "-DCHECK_SETTINGS=${CHECK_SETTINGS}"
        "-DINPUT_NAME=${INPUT_NAME}"
        "-DINPUT_VALUE=${INPUT_VALUE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error_output
)

if(EXPECT_SUCCESS AND NOT result EQUAL 0)
    message(FATAL_ERROR "Expected configuration to pass, but it failed:\n${output}\n${error_output}")
elseif(NOT EXPECT_SUCCESS AND result EQUAL 0)
    message(FATAL_ERROR "Expected configuration to fail, but it passed.")
elseif(NOT EXPECT_SUCCESS AND NOT error_output MATCHES "${EXPECT_ERROR}")
    message(FATAL_ERROR "Configuration failed for an unexpected reason:\n${output}\n${error_output}")
endif()
