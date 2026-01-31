# SAPF Unit Tests using Google Test

include(GoogleTest)

# Common function to set up a unit test executable
function(add_sapf_unit_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE
        sapf_engine
        GTest::gtest_main
    )
    target_include_directories(${TEST_NAME} PRIVATE
        ${SAPF_ROOT}/include
        ${SAPF_ROOT}/src/engine
    )
    # Ensure C++17 standard
    target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
    gtest_discover_tests(${TEST_NAME})
endfunction()

# Reference counting tests
add_sapf_unit_test(test_refcount test_refcount.cpp)

# Value (V class) tests
add_sapf_unit_test(test_value test_value.cpp)

# Symbol table tests
add_sapf_unit_test(test_symbol test_symbol.cpp)

# Hash function tests
add_sapf_unit_test(test_hash test_hash.cpp)

# Parser tests
add_sapf_unit_test(test_parser test_parser.cpp)

# VM opcode tests
add_sapf_unit_test(test_vm test_vm.cpp)

# Array/List tests
add_sapf_unit_test(test_array_list test_array_list.cpp)

# Error handling tests
add_sapf_unit_test(test_errors test_errors.cpp)
