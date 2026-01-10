# CMake script to run sapf with a test file
# Variables:
#   SAPF_BINARY - path to sapf executable
#   TEST_FILE - path to the test file to run

if(NOT EXISTS "${TEST_FILE}")
    message(FATAL_ERROR "Test file not found: ${TEST_FILE}")
endif()

# Create a temp file that includes the test file and then quits
set(TEMP_INPUT "${CMAKE_CURRENT_LIST_DIR}/temp_input.txt")
file(WRITE "${TEMP_INPUT}" "\"${TEST_FILE}\" load\nquit\n")

# Run sapf
execute_process(
    COMMAND ${SAPF_BINARY}
    INPUT_FILE "${TEMP_INPUT}"
    OUTPUT_VARIABLE OUTPUT
    ERROR_VARIABLE ERROR_OUTPUT
    RESULT_VARIABLE RESULT
    TIMEOUT 60
)

# Clean up temp file
file(REMOVE "${TEMP_INPUT}")

# Check result
if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "sapf exited with code ${RESULT}\nOutput:\n${OUTPUT}\nError:\n${ERROR_OUTPUT}")
endif()

# Check for "FAILED" in output
if(OUTPUT MATCHES "FAILED")
    message(FATAL_ERROR "Tests failed:\n${OUTPUT}")
endif()

message(STATUS "All tests passed")
