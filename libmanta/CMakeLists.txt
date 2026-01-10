
set(MANTA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(MANTA_SOURCES
	${MANTA_SOURCE_DIR}/Manta.cpp
	${MANTA_SOURCE_DIR}/MantaMulti.cpp
	${MANTA_SOURCE_DIR}/MantaUSB.cpp
)

# Platform-specific HID backend
if(APPLE)
	list(APPEND MANTA_SOURCES ${MANTA_SOURCE_DIR}/extern/hidapi/mac/hid.c)
	set(MANTA_SUPPORTED TRUE)
elseif(UNIX AND NOT WIN32)
	list(APPEND MANTA_SOURCES ${MANTA_SOURCE_DIR}/extern/hidapi/libusb/hid.c)
	set(MANTA_SUPPORTED TRUE)
else()
	# Windows: no hidapi backend available, create stub library
	set(MANTA_SUPPORTED FALSE)
endif()

add_library(
	manta
	STATIC
	${MANTA_SOURCES}
)

target_include_directories(
	manta
	PUBLIC
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}
	${MANTA_SOURCE_DIR}/extern/hidapi/hidapi
)

if(APPLE)
	target_link_libraries(
		manta
		PUBLIC
		"-framework IOKit"
		"-framework CoreFoundation"
	)
elseif(UNIX AND NOT WIN32)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
	target_include_directories(manta PRIVATE ${LIBUSB_INCLUDE_DIRS})
	target_link_libraries(manta PUBLIC ${LIBUSB_LIBRARIES} pthread)
endif()

# Export whether Manta is supported
set(MANTA_SUPPORTED ${MANTA_SUPPORTED} PARENT_SCOPE)
