cmake_minimum_required (VERSION 3.1)
project (libacars C)

set (LIBACARS_VERSION_MAJOR 1)
set (LIBACARS_VERSION_MINOR 0)
set (LIBACARS_VERSION_PATCH 0)
set (LIBACARS_VERSION
	"${LIBACARS_VERSION_MAJOR}.${LIBACARS_VERSION_MINOR}.${LIBACARS_VERSION_PATCH}"
)
set (CMAKE_C_STANDARD 11)
set (C_STANDARD_REQUIRED ON)
set (CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
   message(STATUS "Build type not specified: defaulting to Release")
endif(NOT CMAKE_BUILD_TYPE)

option(EMIT_ASN_DEBUG, "Enable debugging of ASN.1 decoder (requires
CMAKE_BUILD_TYPE=Debug" OFF)

add_definitions(-D_XOPEN_SOURCE=600)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG=1")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DDEBUG=0")

include_directories (
	${PROJECT_SOURCE_DIR}/src
	"${PROJECT_SOURCE_DIR}/src/asn1"
)

add_subdirectory (src)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	@ONLY
)

#only add uninstall target if this is the top project
if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()