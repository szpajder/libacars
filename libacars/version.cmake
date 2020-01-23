set (VERSION "char const * const LA_VERSION=\"")
execute_process(COMMAND git describe --always --tags --dirty
		OUTPUT_VARIABLE GIT_VERSION
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)

if ("${GIT_VERSION}" STREQUAL "")
	string(APPEND VERSION "${LA_VERSION}")
elseif("${GIT_VERSION}" MATCHES ".+-g(.+)")
	string(APPEND VERSION "${LA_VERSION}-${CMAKE_MATCH_1}")
elseif("${GIT_VERSION}" MATCHES "v(.+)")
	string(APPEND VERSION "${CMAKE_MATCH_1}")
else()
	string(APPEND VERSION "${LA_VERSION}-${GIT_VERSION}")
endif()
string(APPEND VERSION "\";\n")

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/version.c)
	file(READ ${CMAKE_CURRENT_BINARY_DIR}/version.c VERSION_)
else()
	set(VERSION_ "")
endif()

if (NOT "${VERSION}" STREQUAL "${VERSION_}")
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.c "${VERSION}")
endif()
