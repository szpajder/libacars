configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/version.h"
	@ONLY
)

add_subdirectory (asn1)
add_library(acars SHARED
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
target_link_libraries (acars asn1 m)
set_property (TARGET acars PROPERTY SOVERSION ${LIBACARS_VERSION_MAJOR})

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
