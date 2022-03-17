# Install script for directory: D:/psi_h/apsi_3_8/APSI/common/apsi

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/APSI-0.7/apsi" TYPE FILE FILES
    "D:/psi_h/apsi_3_8/APSI/common/apsi/apsi.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/crypto_context.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/item.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/log.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/powers.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/psi_params.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/requests.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/responses.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/seal_object.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/thread_pool_mgr.h"
    "D:/psi_h/apsi_3_8/APSI/common/apsi/version.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/psi_h/apsi_3_8/APSI/out/build/x64-Debug/common/apsi/fourq/cmake_install.cmake")
  include("D:/psi_h/apsi_3_8/APSI/out/build/x64-Debug/common/apsi/network/cmake_install.cmake")
  include("D:/psi_h/apsi_3_8/APSI/out/build/x64-Debug/common/apsi/oprf/cmake_install.cmake")
  include("D:/psi_h/apsi_3_8/APSI/out/build/x64-Debug/common/apsi/util/cmake_install.cmake")

endif()

