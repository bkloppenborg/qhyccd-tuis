CMAKE_MINIMUM_REQUIRED(VERSION 3.22)

INCLUDE(Version)

##
# Activate (optional) generators
##
# Source package
SET(CPACK_SOURCE_GENERATOR "")
OPTION(CREATE_TGZ "Create a .tgz install file" OFF)
IF(${CREATE_TGZ})
  LIST(APPEND CPACK_GENERATOR "TGZ")
ENDIF()

# Self-extracting installer for *nix operating systems
OPTION(CREATE_STGZ "Create .sh install file" OFF)
MARK_AS_ADVANCED(CREATE_STGZ)
IF(${CREATE_STGZ})
  LIST(APPEND CPACK_GENERATOR "STGZ")
ENDIF()

# Debian packaging
OPTION(CREATE_DEB "Create .deb install file" ON)
MARK_AS_ADVANCED(CREATE_DEB)
IF(${CREATE_DEB})
  LIST(APPEND CPACK_GENERATOR "DEB")
  SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${PROCESSOR_ARCHITECTURE})
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-base (>= 6.4), libopencv (>= 4.6), libcfitsio10 (>= 4.2)")
ENDIF()

# RPM packaging
OPTION(CREATE_RPM "Create .rpm install file" OFF)
MARK_AS_ADVANCED(CREATE_RPM)
IF(${CREATE_RPM})
  LIST(APPEND CPACK_GENERATOR "RPM")
  SET(CPACK_RPM_PACKAGE_LICENSE "BSD 3-Clause)")
  SET(CPACK_RPM_PACKAGE_AUTOREQPROV " no")
ENDIF()

##
# Common information to all packaging tools
##
SET(CPACK_PREFIX_DIR ${CMAKE_INSTALL_PREFIX})
SET(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
SET(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
SET(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
SET(CPACK_PACKAGE_VENDOR ${VENDOR_NAME})
SET(CPACK_PACKAGE_CONTACT ${VENDOR_CONTACT})
SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
SET(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")

# Long description of the package
SET(CPACK_PACKAGE_DESCRIPTION "Text user interfaces for QHYCCD cameras")

# Short description of the package
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Text user interfaces for QHYCCD cameras")

SET(CPACK_PACKAGE_GROUP "Development/Software")

# Useful descriptions for components
SET(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "QHYCCD Text User Interfaces")
SET(CPACK_COMPONENT_DOCUMENTATION_NAME "Doxygen documentation")
SET(CPACK_COMPONENT_HEADERS_NAME "C/C++ headers")
SET(CPACK_COMPONENT_CMAKE_NAME "CMake support")
# Set the default components installed in the package
SET(CPACK_COMPONENTS_ALL libraries headers documentation cmake)

# Naming convention for the package, this  is probably ok in most cases
SET(CPACK_SOURCE_PACKAGE_FILE_NAME
  ${CPACK_PACKAGE_NAME}_src_${CPACK_PACKAGE_VERSION}_${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR})

# A standard list of files to ignore.
SET(CPACK_SOURCE_IGNORE_FILES
    "/build"
    "CMakeFiles"
    "/\\\\.dir"
    "/\\\\.git"
    "/\\\\.gitignore$"
    ".*~$"
    "\\\\.bak$"
    "\\\\.swp$"
    "\\\\.orig$"
    "/\\\\.DS_Store$"
    "/Thumbs\\\\.db"
    "/CMakeLists.txt.user$"
    ${CPACK_SOURCE_IGNORE_FILES})

# Ignore build directories that may be in the source tree
FILE(GLOB_RECURSE CACHES "${CMAKE_SOURCE_DIR}/CMakeCache.txt")

# Call to CPACK
INCLUDE(CPack)
