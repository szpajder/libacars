# FindZLIB.cmake often finds the path to the static library (libz.dll.a)
# This macro searches for the actual DLL in the same set of places
# (assuming it's named zlib1.dll)
macro(find_zlibdll)
    set(_ZLIBDLL_SEARCHES)

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
endmacro()
