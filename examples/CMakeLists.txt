# Host-driven audio: the embedding app owns the audio callback.
add_executable(host_audio host_audio.cpp)
target_link_libraries(host_audio PRIVATE sapf_engine)
target_compile_features(host_audio PRIVATE cxx_std_17)
add_test(NAME example_host_audio COMMAND host_audio)

# Link check: the static engine must link into a shared library (requires PIC on Linux)
add_library(host_audio_module MODULE host_audio.cpp)
target_link_libraries(host_audio_module PRIVATE sapf_engine)
target_compile_features(host_audio_module PRIVATE cxx_std_17)

option(SAPF_BUILD_CLAP "Build the sapf CLAP plugin example" ON)
if(SAPF_BUILD_CLAP)
	add_subdirectory(clap)
endif()
