#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "APSI::apsi" for configuration "Debug"
set_property(TARGET APSI::apsi APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(APSI::apsi PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/apsi-0.7.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS APSI::apsi )
list(APPEND _IMPORT_CHECK_FILES_FOR_APSI::apsi "${_IMPORT_PREFIX}/lib/apsi-0.7.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
