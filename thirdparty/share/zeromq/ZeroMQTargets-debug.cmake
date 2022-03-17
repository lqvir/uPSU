#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libzmq-static" for configuration "Debug"
set_property(TARGET libzmq-static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libzmq-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX;RC"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "ws2_32;advapi32;rpcrt4;iphlpapi"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/libzmq-mt-sgd-4_3_4.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS libzmq-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_libzmq-static "${_IMPORT_PREFIX}/debug/lib/libzmq-mt-sgd-4_3_4.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
