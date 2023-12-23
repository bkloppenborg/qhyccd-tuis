# - Find QHYCCD Drivers

# Defines the following variables:
# QHYCCD_INCLUDE_DIRS    - Location of QHYCCD's include directories
# QHYCCD_LIBRARIES       - Location of QHYCCD's libraries
# QHYCCD_FOUND           - True if QHYCCD has been located
# QHYCCD_MODULES         - Location of QHYCCD's Fortran modules
#
# You may provide a hint to where QHYCCD's root directory may be located
# by setting QHYCCD_ROOT before calling this script.
#
# Variables used by this module, they can change the default behaviour and
# need to be set before calling find_package:
#
#=============================================================================
# Copyright 2023 Brian Kloppenborg
#
#  This code is licensed under the MIT License.  See the FindQHYCCD.cmake script
#  for the text of the license.
#
# The MIT License
#
# License for the specific language governing rights and limitations under
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#=============================================================================

IF(QHYCCD_INCLUDES)
  # Already in cache, be silent
  set (QHYCCD_FIND_QUIETLY TRUE)
ENDIF (QHYCCD_INCLUDES)

find_path(QHYCCD_INCLUDE_DIR
  NAMES "qhyccd.h"
  PATHS ${QHYCCD_ROOT}
        ${CMAKE_SYSTEM_INCLUDE_PATH}
        ${CMAKE_SYSTEM_PREFIX_PATH}
  PATH_SUFFIXES "include"
  DOC "Root directory for QHYCCD header file."
)

find_library(QHYCCD_LIBRARY
  NAMES "qhyccd"
  PATHS ${QHYCCD_ROOT}
        ${CMAKE_SYSTEM_PREFIX_PATH}
  PATH_SUFFIXES "lib"
  DOC "QHYCCD library"
)

find_package(OpenCV REQUIRED)

mark_as_advanced(QHYCCD_INCLUDE_DIR QHYCCD_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QHYCCD DEFAULT_MSG
  QHYCCD_INCLUDE_DIR QHYCCD_LIBRARY)

if (QHYCCD_FOUND)
  add_library(QHYCCD::QHYCCD UNKNOWN IMPORTED)
  set_target_properties(QHYCCD::QHYCCD PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGE "C"
    IMPORTED_LOCATION "${QHYCCD_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${QHYCCD_INCLUDE_DIRS}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${OpenCV_LIBS}")
endif (QHYCCD_FOUND)