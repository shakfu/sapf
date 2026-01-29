cmake_minimum_required(VERSION 3.15)

project(sapf_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Optional: Disables compiler extensions beyond the standard

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


add_subdirectory(libmanta)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "Disable rtaudio tests" FORCE)
add_subdirectory(thirdparty/rtaudio)
set(RTMIDI_BUILD_TESTING OFF CACHE BOOL "Disable rtmidi tests" FORCE)
set(RTMIDI_TARGETNAME_UNINSTALL "rtmidi_uninstall" CACHE STRING "Unique uninstall target for rtmidi" FORCE)
add_subdirectory(thirdparty/rtmidi)

# Replxx for cross-platform line editing
set(REPLXX_BUILD_EXAMPLES OFF CACHE BOOL "Disable replxx examples" FORCE)
add_subdirectory(thirdparty/replxx)

# FFTW for non-Apple platforms
if(NOT APPLE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static FFTW" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "Disable FFTW tests" FORCE)
    set(DISABLE_FORTRAN ON CACHE BOOL "Disable Fortran wrappers" FORCE)
    add_subdirectory(thirdparty/fftw-3.3.10)
endif()

add_subdirectory(src/engine)
add_subdirectory(src/cli)

# Testing
enable_testing()
add_subdirectory(tests)
