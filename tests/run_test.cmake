# CMake script to run a sapf test with inline input
# Variables:
#   SAPF_BINARY - path to sapf executable
#   INPUT - sapf code to execute
#   EXPECTED_PATTERN - regex pattern to match in output (optional)

# Create temp input file with the test input and quit command
set(TEMP_INPUT "${CMAKE_CURRENT_LIST_DIR}/temp_input.txt")
file(WRITE "${TEMP_INPUT}" "${INPUT}\nquit\n")

# Run sapf
execute_process(
    COMMAND ${SAPF_BINARY}
    INPUT_FILE "${TEMP_INPUT}"
    OUTPUT_VARIABLE OUTPUT
    ERROR_VARIABLE ERROR_OUTPUT
    RESULT_VARIABLE RESULT
    TIMEOUT 10
)

# Clean up temp file
file(REMOVE "${TEMP_INPUT}")

# Check result
if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "sapf exited with code ${RESULT}\nOutput:\n${OUTPUT}\nError:\n${ERROR_OUTPUT}")
endif()

# Check output pattern if specified
if(DEFINED EXPECTED_PATTERN)
    if(NOT OUTPUT MATCHES "${EXPECTED_PATTERN}")
        message(FATAL_ERROR "Expected pattern '${EXPECTED_PATTERN}' not found in output:\n${OUTPUT}")
    endif()
endif()

message(STATUS "Test passed")
