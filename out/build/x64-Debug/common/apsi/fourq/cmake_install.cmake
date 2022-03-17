# Install script for directory: D:/psi_h/apsi_3_8/APSI/common/apsi/fourq

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/psi_h/apsi_3_8/APSI/out/install/x64-Debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/APSI-0.7/apsi/fourq" TYPE FILE FILES
    "D:/psi_h/apsi_3_8/APSI/common/apsi/fourq/FourQ_api.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/fourq/FourQ_internal.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/fourq/FourQ_params.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/fourq/FourQ_tables.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/fourq/FourQ.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/fourq/table_lookup.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/psi_h/apsi_3_8/APSI/out/build/x64-Debug/common/apsi/fourq/amd64/cmake_install.cmake")

endif()

