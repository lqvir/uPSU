# Install script for directory: D:/psi_h/apsi_3_8/APSI

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/psi_h/apsi_3_8/APSI/out/install/x64-Release")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "release")
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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/APSI-0.7/apsi" TYPE FILE FILES "D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/common/apsi/config.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/lib/apsi-0.7.lib")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/APSI-0.7/APSITargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/APSI-0.7/APSITargets.cmake"
         "D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/CMakeFiles/Export/lib/cmake/APSI-0.7/APSITargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/APSI-0.7/APSITargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/APSI-0.7/APSITargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/APSI-0.7" TYPE FILE FILES "D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/CMakeFiles/Export/lib/cmake/APSI-0.7/APSITargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/APSI-0.7" TYPE FILE FILES "D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/CMakeFiles/Export/lib/cmake/APSI-0.7/APSITargets-release.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/APSI-0.7" TYPE FILE FILES
    "D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/cmake/APSIConfig.cmake"
    "D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/cmake/APSIConfigVersion.cmake"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/common/apsi/cmake_install.cmake")
  include("D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/receiver/apsi/cmake_install.cmake")
  include("D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/sender/apsi/cmake_install.cmake")
  include("D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/cli/common/cmake_install.cmake")
  include("D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/cli/sender/cmake_install.cmake")
  include("D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/cli/receiver/cmake_install.cmake")
  include("D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/cli/pd_tool/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "D:/psi_h/apsi_3_8/APSI/out/build/x64-Release/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
