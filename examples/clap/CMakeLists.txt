# sapf CLAP plugin: ImGui editor (pugl + OpenGL) and loopback UDP input.
add_subdirectory(${SAPF_ROOT}/thirdparty/pugl ${CMAKE_BINARY_DIR}/thirdparty/pugl)
add_subdirectory(${SAPF_ROOT}/thirdparty/imgui ${CMAKE_BINARY_DIR}/thirdparty/imgui)

# Embed sapf-prelude.txt: a DAW does not inherit SAPF_PRELUDE from the shell.
# A byte array, since MSVC limits string literals to 64 KB.
set(SAPF_PRELUDE_TXT ${SAPF_ROOT}/sapf-prelude.txt)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SAPF_PRELUDE_TXT})
file(READ ${SAPF_PRELUDE_TXT} SAPF_PRELUDE_HEX HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," SAPF_PRELUDE_BYTES "${SAPF_PRELUDE_HEX}")
file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/prelude_data.cpp
	CONTENT "// Generated from sapf-prelude.txt by examples/clap/CMakeLists.txt\nextern const char kSapfPrelude[] = {${SAPF_PRELUDE_BYTES}0x00};\n")

add_library(sapf_clap MODULE sapf_clap.cpp udp_server.cpp editor_imgui.cpp editor_logic.cpp
	${CMAKE_CURRENT_BINARY_DIR}/prelude_data.cpp)
target_link_libraries(sapf_clap PRIVATE sapf_engine pugl_plugin imgui_gl3)
target_include_directories(sapf_clap PRIVATE ${SAPF_ROOT}/thirdparty/clap/include)
target_compile_features(sapf_clap PRIVATE cxx_std_17)
set_target_properties(sapf_clap PROPERTIES OUTPUT_NAME sapf CXX_VISIBILITY_PRESET hidden)
if(APPLE)
	target_compile_definitions(sapf_clap PRIVATE GL_SILENCE_DEPRECATION)
elseif(WIN32)
	target_compile_definitions(sapf_clap PRIVATE NOMINMAX) # <windows.h> via pugl/gl.h
endif()

if(APPLE)
	target_link_options(sapf_clap PRIVATE -exported_symbols_list ${SAPF_ROOT}/thirdparty/clap/src/macos-symbols.txt)
	set_target_properties(sapf_clap PROPERTIES
		BUNDLE TRUE
		BUNDLE_EXTENSION clap
		MACOSX_BUNDLE_GUI_IDENTIFIER com.github.shakfu.sapf
		MACOSX_BUNDLE_BUNDLE_NAME sapf
		MACOSX_BUNDLE_BUNDLE_VERSION 0.1.22
		MACOSX_BUNDLE_SHORT_VERSION_STRING 0.1.22
		MACOSX_BUNDLE_INFO_PLIST ${SAPF_ROOT}/thirdparty/clap/src/plugins.plist.in)
else()
	set_target_properties(sapf_clap PROPERTIES PREFIX "" SUFFIX ".clap")
	if(WIN32)
		target_link_libraries(sapf_clap PRIVATE ws2_32)
	endif()
endif()

# Interactive host: the plugin's editor in a window, audio to the default output (`make editor`).
add_executable(sapf_dev_host dev_host.cpp)
target_link_libraries(sapf_dev_host PRIVATE pugl_host rtaudio ${CMAKE_DL_LIBS})
target_include_directories(sapf_dev_host PRIVATE ${SAPF_ROOT}/thirdparty/clap/include ${SAPF_ROOT}/thirdparty/rtaudio)
target_compile_features(sapf_dev_host PRIVATE cxx_std_17)
target_compile_definitions(sapf_dev_host PRIVATE SAPF_CLAP_PLUGIN_PATH="$<TARGET_FILE:sapf_clap>")
add_dependencies(sapf_dev_host sapf_clap)

# Headless host: loads the built plugin, drives it over UDP, renders audio.
add_executable(test_clap_host test_clap_host.cpp)
target_include_directories(test_clap_host PRIVATE ${SAPF_ROOT}/thirdparty/clap/include)
target_compile_features(test_clap_host PRIVATE cxx_std_17)
add_dependencies(test_clap_host sapf_clap)
if(WIN32)
	target_link_libraries(test_clap_host PRIVATE ws2_32)
elseif(NOT APPLE)
	target_link_libraries(test_clap_host PRIVATE ${CMAKE_DL_LIBS})
endif()
add_test(NAME example_clap_plugin COMMAND test_clap_host $<TARGET_FILE:sapf_clap>)
set_tests_properties(example_clap_plugin PROPERTIES ENVIRONMENT "SAPF_CLAP_UDP_PORT=57199" TIMEOUT 60)

include(GoogleTest)
add_executable(test_editor_logic test_editor_logic.cpp editor_logic.cpp)
target_link_libraries(test_editor_logic PRIVATE GTest::gtest_main)
target_compile_features(test_editor_logic PRIVATE cxx_std_17)
gtest_discover_tests(test_editor_logic)
