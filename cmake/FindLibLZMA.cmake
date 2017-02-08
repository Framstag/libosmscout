# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindLibLZMA
# -----------
#
# Find LibLZMA
#
# Find LibLZMA headers and library
#
# ::
#
#   LIBLZMA_FOUND             - True if liblzma is found.
#   LIBLZMA_INCLUDE_DIRS      - Directory where liblzma headers are located.
#   LIBLZMA_LIBRARIES         - Lzma libraries to link against.

find_path(LIBLZMA_INCLUDE_DIRS lzma.h )
find_library(LIBLZMA_LIBRARIES lzma)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBLZMA DEFAULT_MSG LIBLZMA_INCLUDE_DIRS LIBLZMA_LIBRARIES)
