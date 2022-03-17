#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SEAL::seal" for configuration "Debug"
set_property(TARGET SEAL::seal APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(SEAL::seal PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/seal-3.7.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS SEAL::seal )
list(APPEND _IMPORT_CHECK_FILES_FOR_SEAL::seal "${_IMPORT_PREFIX}/debug/lib/seal-3.7.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
