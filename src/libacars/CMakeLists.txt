include(CheckCSourceCompiles)

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

if (HAVE_LD_VERSION_SCRIPT)
	set(acars_ldscript "${CMAKE_CURRENT_SOURCE_DIR}/symversions.ldscript")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--version-script,${acars_ldscript}")
endif()

find_library(LIBM m REQUIRED)

option(MIAM "Enable decoder of MIAM messages (requires zlib)" ON)
set(WITH_MIAM FALSE)

if(MIAM)
	find_package(ZLIB)
	if(ZLIB_FOUND)
		set(WITH_MIAM TRUE)
		list(APPEND acars_extra_sources miam.c miam-core.c)
		list(APPEND acars_extra_install_files miam.h miam-core.h)
		list(APPEND acars_extra_libs ${ZLIB_LIBRARIES})
		list(APPEND acars_include_dirs ${ZLIB_INCLUDE_DIRS})
	endif()
endif()

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.c
		${CMAKE_CURRENT_BINARY_DIR}/_version.c
	COMMAND ${CMAKE_COMMAND} -DLA_VERSION=${LA_VERSION} -P ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake
)

message(STATUS "libacars configuration summary:")
message(STATUS "- MIAM:\trequested: ${MIAM}\tenabled: ${WITH_MIAM}")

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/config.h"
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
	util.c
	vstring.c
	${CMAKE_CURRENT_BINARY_DIR}/version.c
	${acars_extra_sources}
)
set_property(TARGET acars_core PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(acars_core PUBLIC ${acars_include_dirs})

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
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
)
install(FILES
	acars.h
	adsc.h
	arinc.h
	cpdlc.h
	libacars.h
	list.h
	media-adv.h
	version.h
	vstring.h
	${acars_extra_install_files}
	DESTINATION include/libacars
)
install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
	DESTINATION lib/pkgconfig
)
install(FILES
	"${PROJECT_SOURCE_DIR}/README.md"
	"${PROJECT_SOURCE_DIR}/LICENSE"
	DESTINATION "share/doc/${PROJECT_NAME}"
)
install(DIRECTORY
	"${PROJECT_SOURCE_DIR}/doc/"
	DESTINATION "share/doc/${PROJECT_NAME}"
)
