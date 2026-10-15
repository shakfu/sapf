cmake_minimum_required(VERSION 3.15)

project(sapf_project)
if(APPLE)
    enable_language(OBJC) # thirdparty/pugl, used by examples/clap
endif()

# Store the sapf root directory for use in subdirectories
set(SAPF_ROOT ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "SAPF root directory")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Optional: Disables compiler extensions beyond the standard

# Static libraries must link into shared-library hosts (Pd externals, VST3/CLAP plugins)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)

option(UNIVERSAL "Build Universal Binaries" OFF)

# use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache in ${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()


message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if (NOT CMAKE_OSX_ARCHITECTURES)
        if(UNIVERSAL)
            set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architecture" FORCE)
        else()
            set(CMAKE_OSX_ARCHITECTURES ${CMAKE_SYSTEM_PROCESSOR} CACHE STRING "macOS architecture" FORCE)
        endif()
        message("CMAKE_OSX_ARCHITECTURES set to ${CMAKE_OSX_ARCHITECTURES}")
    endif()
endif()


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")


# The engine library and FFTW only, for a host that embeds sapf and drives its audio (MaxAudioBackend)
option(SAPF_ENGINE_ONLY "Build only the engine library" OFF)
if(SAPF_ENGINE_ONLY)
    set(SAPF_REPLXX OFF)  # normal variables: the options below keep their cached values (CMP0077)
    set(SAPF_USE_RTAUDIO OFF)
    set(SAPF_USE_RTMIDI OFF)
    set(SAPF_USE_ALSA OFF)
endif()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)

if(NOT SAPF_ENGINE_ONLY)
    add_subdirectory(libmanta)
    set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "Disable rtaudio tests" FORCE)
    add_subdirectory(thirdparty/rtaudio)
    set(RTMIDI_BUILD_TESTING OFF CACHE BOOL "Disable rtmidi tests" FORCE)
    set(RTMIDI_TARGETNAME_UNINSTALL "rtmidi_uninstall" CACHE STRING "Unique uninstall target for rtmidi" FORCE)
    add_subdirectory(thirdparty/rtmidi)
endif()

# Replxx for the REPL's line editing
option(SAPF_REPLXX "Line editing with replxx" ON)
if(SAPF_REPLXX)
    set(REPLXX_BUILD_EXAMPLES OFF CACHE BOOL "Disable replxx examples" FORCE)
    add_subdirectory(thirdparty/replxx)
endif()

# FFTW for non-Apple platforms (Linux and Windows)
if(NOT APPLE)
    set(BUILD_TESTS OFF CACHE BOOL "Disable FFTW tests" FORCE)
    set(DISABLE_FORTRAN ON CACHE BOOL "Disable Fortran wrappers" FORCE)
    # Enable SSE2 for better performance on x86/x64
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64|AMD64|i686)")
        set(ENABLE_SSE2 ON CACHE BOOL "Enable SSE2 for FFTW" FORCE)
    endif()
    add_subdirectory(thirdparty/fftw-3.3.10)
endif()

add_subdirectory(src/engine)
if(SAPF_ENGINE_ONLY)
    return()
endif()
add_subdirectory(src/cli)

# Testing
enable_testing()

# Google Test for unit tests (vendored in thirdparty/)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(thirdparty/googletest)

add_subdirectory(tests)
add_subdirectory(examples)
