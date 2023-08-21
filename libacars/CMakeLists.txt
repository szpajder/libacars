include(CheckCSourceCompiles)
include(CheckCCompilerFlag)
include(TestBigEndian)
include(CheckSymbolExists)
include(CheckPrototypeDefinition)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

# Use ZLIB_ROOT when searching for zlib with find_package
# This is used when making Win32 binary releases.
cmake_policy(SET CMP0074 NEW)

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

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# strdup, strtok_r
add_definitions(-D_XOPEN_SOURCE=600)

# strnlen
add_definitions(-D_POSIX_C_SOURCE=200809L)

# strsep
# Also silence warnings about obsolete _BSD_SOURCE macro in ASN.1 code
add_definitions(-D_DEFAULT_SOURCE)

# strsep on MacOS
if(APPLE)
	add_definitions(-D_DARWIN_C_SOURCE)
endif()

if (HAVE_LD_VERSION_SCRIPT)
	set(acars_ldscript "${CMAKE_CURRENT_SOURCE_DIR}/symversions.ldscript")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--version-script,${acars_ldscript}")
endif()

CHECK_C_COMPILER_FLAG(-fno-common CC_HAS_FNO_COMMON)
if(CC_HAS_FNO_COMMON)
        add_definitions(-fno-common)
endif()

TEST_BIG_ENDIAN(IS_BIG_ENDIAN)

set(CMAKE_REQUIRED_FLAGS_SAVE ${CMAKE_REQUIRED_FLAGS})
CHECK_SYMBOL_EXISTS(strsep string.h HAVE_STRSEP)
set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_SAVE})

set(CMAKE_REQUIRED_DEFINITIONS_SAVE ${CMAKE_REQUIRED_DEFINITIONS})
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} "-D_GNU_SOURCE")
CHECK_SYMBOL_EXISTS(memmem string.h HAVE_MEMMEM)
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS_SAVE})

# lfind - Unix prototype
check_prototype_definition(
	lfind
	"void *lfind(const void *key, const void *base, size_t *nmemb,
	size_t size, int(*compar)(const void *, const void *))"
	"NULL"
	"stddef.h;search.h"
	LFIND_NMEMB_SIZE_SIZE_T)

# lfind - Windows prototype
check_prototype_definition(
	lfind
	"void *lfind(const void *key, const void *base, unsigned int *nmemb,
	unsigned int size, int(*compar)(const void *, const void *))"
	"NULL"
	"stddef.h;search.h"
	LFIND_NMEMB_SIZE_UINT)

if(NOT ${LFIND_NMEMB_SIZE_SIZE_T} AND NOT ${LFIND_NMEMB_SIZE_UINT})
	message(FATAL_ERROR "Unable to determine lfind function prototype")
endif()

CHECK_INCLUDE_FILE("sys/time.h" HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE("unistd.h" HAVE_UNISTD_H)

if(NOT MSVC)
	find_library(LIBM m REQUIRED)
	list(APPEND acars_extra_libs m)
endif()

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

option(LIBXML2 "Enable libxml2 support for pretty-printing XML messages" ON)
set(WITH_LIBXML2 FALSE)

if(LIBXML2)
	find_package(LibXml2)
	if(LibXml2_FOUND)
		set(WITH_LIBXML2 TRUE)
		list(APPEND acars_extra_libs ${LIBXML2_LIBRARIES})
		list(APPEND acars_include_dirs ${LIBXML2_INCLUDE_DIR})
	endif()
endif()

option(JANSSON "Enable jansson library support for parsing JSON-encoded messages (eg. OHMA)" ON)
set(WITH_JANSSON FALSE)

if(JANSSON)
	find_package(Jansson)
	if(JANSSON_FOUND)
		set(WITH_JANSSON TRUE)
		list(APPEND acars_extra_libs ${JANSSON_LIBRARIES})
		list(APPEND acars_include_dirs ${JANSSON_INCLUDE_DIR})
	endif()
endif()

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.c
		${CMAKE_CURRENT_BINARY_DIR}/_version.c
	COMMAND ${CMAKE_COMMAND} -DLA_VERSION=${LA_VERSION} -P ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake
)

message(STATUS "libacars configuration summary:")
message(STATUS "- zlib:\t\trequested: ${ZLIB}, enabled: ${WITH_ZLIB}")
message(STATUS "- libxml2:\t\trequested: ${LIBXML2}, enabled: ${WITH_LIBXML2}")
message(STATUS "- jansson:\t\trequested: ${JANSSON}, enabled: ${WITH_JANSSON}")

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
	asn1-format-cpdlc-text.c
	asn1-format-cpdlc-json.c
	asn1-util.c
	bitstream.c
	configuration.c
	cpdlc.c
	crc.c
	dict.c
	hash.c
	json.c
	libacars.c
	list.c
	media-adv.c
	miam.c
	miam-core.c
	ohma.c
	reassembly.c
	util.c
	vstring.c
	${CMAKE_CURRENT_BINARY_DIR}/version.c
)
set_property(TARGET acars_core PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(acars_core PUBLIC ${acars_include_dirs} ${CMAKE_CURRENT_BINARY_DIR})

set(acars_obj_files $<TARGET_OBJECTS:asn1> $<TARGET_OBJECTS:acars_core>)

add_library(acars SHARED ${acars_obj_files})
target_link_libraries (acars ${acars_extra_libs})
set_property (TARGET acars PROPERTY SOVERSION ${LA_VERSION_MAJOR})
set_target_properties(acars PROPERTIES OUTPUT_NAME "acars-${LA_VERSION_MAJOR}")
if (HAVE_LD_VERSION_SCRIPT)
	set_property (TARGET acars PROPERTY LINK_DEPENDS ${acars_ldscript})
endif()

add_library(acars_static ${acars_obj_files})
if(MSVC)
    # In order to build with ninja, we need static and shared .lib names to be different
    set_target_properties(acars_static PROPERTIES OUTPUT_NAME "acarsstatic-${LA_VERSION_MAJOR}")
else()
    set_target_properties(acars_static PROPERTIES OUTPUT_NAME "acars-${LA_VERSION_MAJOR}")
endif()

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-${LA_VERSION_MAJOR}.pc"
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
	asn1-format-common.h
	asn1-util.h
	cpdlc.h
	dict.h
	hash.h
	json.h
	libacars.h
	list.h
	media-adv.h
	miam.h
	miam-core.h
	ohma.h
	reassembly.h
	version.h
	vstring.h
	DESTINATION
	"${CMAKE_INSTALL_INCLUDEDIR}/${LA_SUBDIR}/${PROJECT_NAME}"
)
install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-${LA_VERSION_MAJOR}.pc"
	DESTINATION
	"${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
install(FILES
	"${PROJECT_SOURCE_DIR}/README.md"
	"${PROJECT_SOURCE_DIR}/LICENSE.md"
	DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${LA_SUBDIR}"
)
install(DIRECTORY
	"${PROJECT_SOURCE_DIR}/doc/"
	DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${LA_SUBDIR}"
)
# Copy zlib1.dll to the bundle on Windows
if(WIN32 AND ZLIB_FOUND
AND NOT ${ZLIBDLL_LIBRARY_RELEASE} STREQUAL "ZLIBDLL_LIBRARY_RELEASE-NOTFOUND")
	install(FILES
		${ZLIBDLL_LIBRARY_RELEASE}
		DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
endif()
