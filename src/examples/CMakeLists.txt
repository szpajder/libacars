set (EXAMPLE_BINARIES
	decode_acars_apps
	adsc_get_position
	cpdlc_get_position
	media_advisory
)
foreach (ex ${EXAMPLE_BINARIES})
	add_executable(${ex} ${ex}.c)
	target_link_libraries(${ex} acars)
endforeach()
target_link_libraries (cpdlc_get_position m)
install (TARGETS
	${EXAMPLE_BINARIES}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
)
