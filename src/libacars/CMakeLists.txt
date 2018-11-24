configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/version.h"
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
	util.c
	vstring.c
)
set_property(TARGET acars_core PROPERTY POSITION_INDEPENDENT_CODE ON)

set(acars_obj_files $<TARGET_OBJECTS:asn1> $<TARGET_OBJECTS:acars_core>)

add_library(acars SHARED ${acars_obj_files})
target_link_libraries (acars)
set_property (TARGET acars PROPERTY SOVERSION ${LIBACARS_VERSION_MAJOR})

add_library(acars_static ${acars_obj_files})
set_target_properties(acars_static PROPERTIES OUTPUT_NAME "acars")

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
	@ONLY
)

install(TARGETS acars DESTINATION lib)
install(FILES
	acars.h
	adsc.h
	arinc.h
	cpdlc.h
	libacars.h
	list.h
	version.h
	vstring.h
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
