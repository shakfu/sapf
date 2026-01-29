option(SAPF_USE_RTAUDIO "Include RtAudio backend" ON)
option(SAPF_USE_RTMIDI "Include RtMidi backend" ON)
option(SAPF_USE_MANTA "Include Manta controller support" OFF)

set(ENGINE_SOURCES
	MidiBackend.cpp
	MidiRouter.cpp
	AudioBackend.cpp
	CoreOps.cpp
	DelayUGens.cpp
	dsp.cpp
	elapsedTime.cpp
	ErrorCodes.cpp
	FilterUGens.cpp
	MathFuns.cpp
	MathOps.cpp
	Midi.cpp
	backends/MaxAudioBackend.cpp
	backends/AlsaAudioBackend.cpp
	backends/CoreMidiBackend.cpp
	backends/NullMidiBackend.cpp
	MultichannelExpansion.cpp
	Object.cpp
	Opcode.cpp
	OscilUGens.cpp
	Parser.cpp
	backends/CoreAudioBackend.cpp
	backends/NullAudioBackend.cpp
	primes.cpp
	RandomOps.cpp
	RCObj.cpp
	SetOps.cpp
	SoundFiles.cpp
	Spectrogram.cpp
	SapfEngine.cpp
	StreamOps.cpp
	symbol.cpp
	Types.cpp
	UGen.cpp
	VM.cpp
)


if(SAPF_USE_RTAUDIO)
	list(APPEND ENGINE_SOURCES backends/RtAudioBackend.cpp)
endif()

if(SAPF_USE_RTMIDI)
	list(APPEND ENGINE_SOURCES backends/RtMidiBackend.cpp)
endif()

# Platform-specific sources
if(APPLE)
	list(APPEND ENGINE_SOURCES makeImage.mm)
else()
	list(APPEND ENGINE_SOURCES makeImageStub.cpp)
endif()

add_library(
	sapf_engine
	STATIC
	${ENGINE_SOURCES}
)

# Add compile definitions for optional features
if(SAPF_USE_MANTA)
	target_compile_definitions(sapf_engine PUBLIC SAPF_USE_MANTA)
endif()

if(SAPF_USE_RTAUDIO)
	target_compile_definitions(sapf_engine PUBLIC SAPF_USE_RTAUDIO)
endif()

if(SAPF_USE_RTMIDI)
	target_compile_definitions(sapf_engine PUBLIC SAPF_USE_RTMIDI)
endif()

target_include_directories(
	sapf_engine
	PUBLIC
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/thirdparty/replxx/include
)

if(SAPF_USE_RTAUDIO)
	target_include_directories(sapf_engine PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/rtaudio)
endif()

if(SAPF_USE_RTMIDI)
	target_include_directories(sapf_engine PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/rtmidi)
endif()

# FFTW include directory for non-Apple platforms
if(NOT APPLE)
	target_include_directories(
		sapf_engine
		PUBLIC
		${CMAKE_SOURCE_DIR}/thirdparty/fftw-3.3.10/api
	)
endif()

# Compiler-specific warning flags
if(MSVC)
	target_compile_options(
		sapf_engine
		PUBLIC
		/W3
	)
else()
	target_compile_options(
		sapf_engine
		PUBLIC
		-Wmissing-field-initializers
		-Wreturn-type
		-Wparentheses
		-Wshadow
		-Wsign-compare
		-Wuninitialized
		-Wno-unused-function
		-Wunused-label
		-Wunused-value
		-Wunused-variable
	)
endif()

# Base libraries
target_link_libraries(sapf_engine PUBLIC replxx)

if(SAPF_USE_MANTA)
	target_link_libraries(sapf_engine PUBLIC manta)
endif()

if(SAPF_USE_RTAUDIO)
	target_link_libraries(sapf_engine PUBLIC rtaudio)
endif()

if(SAPF_USE_RTMIDI)
	target_link_libraries(sapf_engine PUBLIC rtmidi)
endif()

# Platform-specific libraries
if(APPLE)
	target_link_libraries(
		sapf_engine
		PUBLIC
		"-framework Accelerate"
		"-framework AudioToolbox"
		"-framework AudioUnit"
		"-framework Carbon"
		"-framework Cocoa"
		"-framework CoreFoundation"
		"-framework CoreMidi"
		"-framework CoreServices"
	)
elseif(UNIX)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(ALSA IMPORTED_TARGET alsa)
	if(ALSA_FOUND)
		target_link_libraries(sapf_engine PUBLIC PkgConfig::ALSA)
	endif()
	target_link_libraries(sapf_engine PUBLIC fftw3 pthread m)
elseif(WIN32)
	target_link_libraries(sapf_engine PUBLIC winmm)
endif()

set_target_properties(
	sapf_engine
	PROPERTIES
	OUTPUT_NAME "sapf"
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
