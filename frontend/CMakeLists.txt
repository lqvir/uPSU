

include_directories(${CMAKE_SOURCE_DIR})

list(APPEND _sources main.cpp util.cpp)

add_executable(frontend_libOTe  ${_sources})

target_link_libraries(frontend_libOTe libOTe_Tests tests_cryptoTools libsodium relic_s bitpolymul.lib cryptoTools libOTe)