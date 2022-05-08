# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

# Source files in this directory
set(APSU_SOURCE_FILES ${APSU_SOURCE_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/bin_bundle.cpp
    ${CMAKE_CURRENT_LIST_DIR}/query.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sender.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sender_db.cpp
)

set(APSU_SOURCE_FILES_SENDER ${APSU_SOURCE_FILES_SENDER}
    ${CMAKE_CURRENT_LIST_DIR}/bin_bundle.cpp
    ${CMAKE_CURRENT_LIST_DIR}/query.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sender.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sender_db.cpp
)

# Add header files for installation
install(
    FILES
        ${CMAKE_CURRENT_LIST_DIR}/bin_bundle.h
        ${CMAKE_CURRENT_LIST_DIR}/query.h
        ${CMAKE_CURRENT_LIST_DIR}/sender.h
        ${CMAKE_CURRENT_LIST_DIR}/sender_db.h
    DESTINATION
        ${APSU_INCLUDES_INSTALL_DIR}/apsu
)

add_subdirectory(util)

if(APSU_USE_ZMQ)
    add_subdirectory(zmq)
endif()
set(APSU_SOURCE_FILES ${APSU_SOURCE_FILES} PARENT_SCOPE)
set(APSU_SOURCE_FILES_SENDER ${APSU_SOURCE_FILES_SENDER} PARENT_SCOPE)

