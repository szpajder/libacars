# FindZLIB.cmake finds the path to the import library (libz.dll.a)
# This macro searches for the actual DLL in the same set of places
# (assuming it's named zlib1.dll)
macro(find_zlibdll)
    set(_ZLIBDLL_SEARCHES)

    # Since cmake 3.17 when using MinGW tools, the find_library() command
    # no longer finds .dll files by default.
    set(CMAKE_FIND_LIBRARY_SUFFIXES_ORIG ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .dll)

    # Search ZLIBDLL_ROOT first if it is set.
    if(ZLIB_ROOT)
      set(_ZLIBDLL_SEARCH_ROOT PATHS ${ZLIB_ROOT} NO_DEFAULT_PATH)
      list(APPEND _ZLIBDLL_SEARCHES _ZLIBDLL_SEARCH_ROOT)
    endif()

    # Normal search.
    set(_ZLIBDLL_SEARCH_NORMAL
      PATHS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\GnuWin32\\Zlib;InstallPath]"
            "$ENV{PROGRAMFILES}/zlib"
      )
    list(APPEND _ZLIBDLL_SEARCHES _ZLIBDLL_SEARCH_NORMAL)

    set(ZLIBDLL_NAMES zlib1)

    foreach(search ${_ZLIBDLL_SEARCHES})
      find_library(ZLIBDLL_LIBRARY_RELEASE NAMES ${ZLIBDLL_NAMES} ${${search}} PATH_SUFFIXES bin)
    endforeach()

    message(STATUS "zlib1.dll path: ${ZLIBDLL_LIBRARY_RELEASE}")

    unset(ZLIBDLL_NAMES)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ORIG})
endmacro()
