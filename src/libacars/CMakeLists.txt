include(CheckCSourceCompiles)
include(TestBigEndian)

# Check if LD supports linker scripts.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/conftest.ldscript" "VERS_1 {
  global: sym;
};

VERS_2 {
  global: sym;
} VERS_1;
")
set(CMAKE_REQUIRED_FLAGS_SAVE ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS} "-Wl,--version-script=${CMAKE_CURRENT_BINARY_DIR}/conftest.ldscript")
check_c_source_compiles("int main(void){return 0;}" HAVE_LD_VERSION_SCRIPT)
set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_SAVE})
file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/conftest.ldscript")

# strdup, strtok_r
add_definitions(-D_XOPEN_SOURCE=600)

# strnlen
add_definitions(-D_POSIX_C_SOURCE=200809L)

if (HAVE_LD_VERSION_SCRIPT)
	set(acars_ldscript "${CMAKE_CURRENT_SOURCE_DIR}/symversions.ldscript")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--version-script,${acars_ldscript}")
endif()

TEST_BIG_ENDIAN(IS_BIG_ENDIAN)

find_library(LIBM m REQUIRED)

option(ZLIB "Enable ZLIB support for MIAM" ON)
set(WITH_ZLIB FALSE)

if(ZLIB)
	find_package(ZLIB)
	if(ZLIB_FOUND)
		set(WITH_ZLIB TRUE)
		list(APPEND acars_extra_libs ${ZLIB_LIBRARIES})
		list(APPEND acars_include_dirs ${ZLIB_INCLUDE_DIRS})
		if(WIN32)
# Locate the actual DLL for inclusion in the installation bundle
			include(${CMAKE_CURRENT_SOURCE_DIR}/find_zlibdll.cmake)
			find_zlibdll()
			if(ZLIBDLL_LIBRARY_RELEASE STREQUAL
					"ZLIBDLL_LIBRARY_RELEASE-NOTFOUND")
				message(WARNING "Failed to locate zlib1.dll - it will not be included in the CPack bundle")
			endif()
		endif()
	endif()
endif()

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.c
		${CMAKE_CURRENT_BINARY_DIR}/_version.c
	COMMAND ${CMAKE_COMMAND} -DLA_VERSION=${LA_VERSION} -P ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake
)

message(STATUS "libacars configuration summary:")
message(STATUS "- ZLIB:\trequested: ${ZLIB}\tenabled: ${WITH_ZLIB}")

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/config.h"
	@ONLY
)

add_subdirectory (asn1)
add_library (acars_core OBJECT
	acars.c
	adsc.c
	arinc.c
	asn1-format-common.c
	asn1-format-cpdlc.c
	asn1-util.c
	bitstream.c
	cpdlc.c
	crc.c
	libacars.c
	list.c
	media-adv.c
	miam.c
	miam-core.c
	util.c
	vstring.c
	${CMAKE_CURRENT_BINARY_DIR}/version.c
)
set_property(TARGET acars_core PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(acars_core PUBLIC ${acars_include_dirs} ${CMAKE_CURRENT_BINARY_DIR})

set(acars_obj_files $<TARGET_OBJECTS:asn1> $<TARGET_OBJECTS:acars_core>)

add_library(acars SHARED ${acars_obj_files})
target_link_libraries (acars m ${acars_extra_libs})
set_property (TARGET acars PROPERTY SOVERSION ${LA_VERSION_MAJOR})
if (HAVE_LD_VERSION_SCRIPT)
	set_property (TARGET acars PROPERTY LINK_DEPENDS ${acars_ldscript})
endif()

add_library(acars_static ${acars_obj_files})
set_target_properties(acars_static PROPERTIES OUTPUT_NAME "acars")

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
	@ONLY
)

install(TARGETS acars
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES
	acars.h
	adsc.h
	arinc.h
	cpdlc.h
	libacars.h
	list.h
	media-adv.h
	miam.h
	miam-core.h
	version.h
	vstring.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)
install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
install(FILES
	"${PROJECT_SOURCE_DIR}/README.md"
	"${PROJECT_SOURCE_DIR}/LICENSE"
	DESTINATION ${CMAKE_INSTALL_DOCDIR}
)
install(DIRECTORY
	"${PROJECT_SOURCE_DIR}/doc/"
	DESTINATION ${CMAKE_INSTALL_DOCDIR}
)
# Copy zlib1.dll to the bundle on Windows
if(WIN32 AND ZLIB_FOUND
AND NOT ${ZLIBDLL_LIBRARY_RELEASE} STREQUAL "ZLIBDLL_LIBRARY_RELEASE-NOTFOUND")
	install(FILES
		${ZLIBDLL_LIBRARY_RELEASE}
		DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
endif()
