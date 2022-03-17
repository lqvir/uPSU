#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "log4cplus::log4cplus" for configuration "Debug"
set_property(TARGET log4cplus::log4cplus APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(log4cplus::log4cplus PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX;RC"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "ws2_32;advapi32"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/log4cplusD.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS log4cplus::log4cplus )
list(APPEND _IMPORT_CHECK_FILES_FOR_log4cplus::log4cplus "${_IMPORT_PREFIX}/debug/lib/log4cplusD.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
