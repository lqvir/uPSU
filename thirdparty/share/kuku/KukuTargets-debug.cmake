#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Kuku::kuku" for configuration "Debug"
set_property(TARGET Kuku::kuku APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(Kuku::kuku PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/libkuku-2.1.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS Kuku::kuku )
list(APPEND _IMPORT_CHECK_FILES_FOR_Kuku::kuku "${_IMPORT_PREFIX}/debug/lib/libkuku-2.1.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
