# Runs the REPL with HOME and the working directory set to an empty DIR, then checks DIR stays empty
# Variables:
#   SAPF_BINARY - path to sapf executable
#   DIR - scratch directory, recreated empty

file(REMOVE_RECURSE "${DIR}")
file(MAKE_DIRECTORY "${DIR}")
set(TEMP_INPUT "${CMAKE_CURRENT_LIST_DIR}/temp_input_no_files.txt")
file(WRITE "${TEMP_INPUT}" "1 2 +\nquit\n")

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env --unset=SAPF_HISTORY --unset=SAPF_LOG HOME=${DIR} USERPROFILE=${DIR} ${SAPF_BINARY}
    WORKING_DIRECTORY "${DIR}"
    INPUT_FILE "${TEMP_INPUT}"
    OUTPUT_VARIABLE OUTPUT
    ERROR_VARIABLE ERROR_OUTPUT
    RESULT_VARIABLE RESULT
    TIMEOUT 10
)
file(REMOVE "${TEMP_INPUT}")

if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "sapf exited with code ${RESULT}\nOutput:\n${OUTPUT}\nError:\n${ERROR_OUTPUT}")
endif()
file(GLOB WRITTEN "${DIR}/*")
if(WRITTEN)
    message(FATAL_ERROR "REPL wrote files: ${WRITTEN}")
endif()
message(STATUS "Test passed")
