# SAPF Tests using CTest

# Unit tests (C++ with Google Test)
add_subdirectory(unit)

# Integration tests (sapf language)

# Smoke test - minimal test to verify sapf starts
add_test(
    NAME smoke
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Split unit tests for better failure isolation
add_test(
    NAME test_equals
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_equals.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_tuples
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_tuples.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_stack
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_stack.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_size_reverse
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_size_reverse.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_conditionals
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_conditionals.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_math
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_math.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_arrays
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_arrays.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_streams
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_streams.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_ordering
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_ordering.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_at
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_at.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_filter
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_filter.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_deep_mapping
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_deep_mapping.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_forms
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_forms.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_inheritance
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_inheritance.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# REPL smoke test: piped stdin, then quit. Exercises the exit path the MIDI tests share.
add_test(
    NAME repl_quit
    COMMAND ${CMAKE_COMMAND}
        -DSAPF_BINARY=$<TARGET_FILE:sapf>
        -DINPUT=
        -DEXPECTED_PATTERN=good\ bye
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_test.cmake
)

# REPL must survive an unwritable log file (fopen returns NULL)
add_test(
    NAME repl_unwritable_log
    COMMAND ${CMAKE_COMMAND}
        -DSAPF_BINARY=$<TARGET_FILE:sapf>
        -DINPUT=1
        -DEXPECTED_PATTERN=good\ bye
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_test.cmake
)
set_tests_properties(repl_unwritable_log PROPERTIES
    ENVIRONMENT "SAPF_LOG=${CMAKE_BINARY_DIR}/no-such-dir/sapf-log.txt")

# With SAPF_HISTORY and SAPF_LOG unset, the REPL writes no files to HOME or the working directory
add_test(
    NAME repl_no_files
    COMMAND ${CMAKE_COMMAND}
        -DSAPF_BINARY=$<TARGET_FILE:sapf>
        -DDIR=${CMAKE_BINARY_DIR}/repl-no-files
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_no_files_test.cmake
)

# MIDI smoke tests (verify MIDI ops don't crash)
# These tests work with both CoreMidi (macOS) and RtMidi (Linux/Windows).
# Hosts without a MIDI subsystem (e.g. CI runners) pass via the "RtMidi error" branch.
add_test(
    NAME midi_list
    COMMAND ${CMAKE_COMMAND}
        -DSAPF_BINARY=$<TARGET_FILE:sapf>
        -DINPUT=midiList
        "-DEXPECTED_PATTERN=(midi sources|RtMidi error)"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_test.cmake
)

add_test(
    NAME midi_start_stop
    COMMAND ${CMAKE_COMMAND}
        -DSAPF_BINARY=$<TARGET_FILE:sapf>
        -DINPUT=midiStart\ midiStop
        "-DEXPECTED_PATTERN=(gMIDIClient|RtMidi initialized|RtMidi error)"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_test.cmake
)

add_test(
    NAME midi_debug
    COMMAND ${CMAKE_COMMAND}
        -DSAPF_BINARY=$<TARGET_FILE:sapf>
        -DINPUT=1\ midiDebug\ 0\ midiDebug
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_test.cmake
)
