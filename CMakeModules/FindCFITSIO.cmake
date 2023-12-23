# - Find SBIG Universal Drivers

# Defines the following variables:
# CFITSIO_INCLUDE_DIRS    - Location of CFITSIO's include directories
# CFITSIO_LIBRARIES       - Location of CFITSIO's libraries
# CFITSIO_FOUND           - True if CFITSIO has been located
# CFITSIO_MODULES         - Location of CFITSIO's Fortran modules
#
# You may provide a hint to where CFITSIO's root directory may be located
# by setting CFITSIO_ROOT before calling this script.
#
# Variables used by this module, they can change the default behaviour and
# need to be set before calling find_package:
#
#=============================================================================
# Copyright 2021 Brian Kloppenborg
#
#  This code is licensed under the MIT License.  See the FindCFITSIO.cmake script
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

IF(CFITSIO_INCLUDES)
  # Already in cache, be silent
  set (CFITSIO_FIND_QUIETLY TRUE)
ENDIF (CFITSIO_INCLUDES)

find_path(CFITSIO_INCLUDE_DIR
  NAMES "fitsio.h"
  PATHS ${CFITSIO_ROOT}
        ${CMAKE_SYSTEM_INCLUDE_PATH}
        ${CMAKE_SYSTEM_PREFIX_PATH}
  PATH_SUFFIXES "include"
  DOC "Root directory for CFITSIO header file."
)

find_library(CFITSIO_LIBRARY
  NAMES "cfitsio"
  PATHS ${CFITSIO_ROOT}
        ${CMAKE_SYSTEM_PREFIX_PATH}
  PATH_SUFFIXES "lib"
  DOC "CFITSIO Library"
)

mark_as_advanced(CFITSIO_INCLUDE_DIR CFITSIO_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CFITSIO DEFAULT_MSG
  CFITSIO_INCLUDE_DIR CFITSIO_LIBRARY)

if (CFITSIO_FOUND)
  add_library(CFITSIO::CFITSIO UNKNOWN IMPORTED)
  set_target_properties(CFITSIO::CFITSIO PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGE "C"
    IMPORTED_LOCATION "${CFITSIO_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CFITSIO_INCLUDE_DIRS}")
endif (CFITSIO_FOUND)
