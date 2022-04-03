set (EXAMPLE_BINARIES
	decode_acars_apps
	adsc_get_position
	cpdlc_get_position
)
foreach (ex ${EXAMPLE_BINARIES})
	add_executable(${ex} ${ex}.c)
	target_link_libraries(${ex} acars)
endforeach()
if(NOT MSVC)
	target_link_libraries (cpdlc_get_position m)
endif()
install (TARGETS
	${EXAMPLE_BINARIES}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
)
