add_executable(
	sapf
	main.cpp
	ReplRunner.cpp
)

target_include_directories(
	sapf
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
	sapf
	PRIVATE
	sapf_engine
)

set_target_properties(
	sapf
	PROPERTIES 
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
