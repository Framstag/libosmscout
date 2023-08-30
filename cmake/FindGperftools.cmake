# - Try to find Gperftools
# Once done, this will define
#
#  GPERFTOOLS_FOUND - system has GPERFTOOLS
#  GPERFTOOLS_INCLUDE_DIRS - the GPERFTOOLS include directories
#  GPERFTOOLS_LIBRARIES - link these to use GPERFTOOLS
#
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_GPERFTOOLS QUIET GPERFTOOLS)

FIND_PATH(GPERFTOOLS_INCLUDE_DIRS
    NAMES gperftools/heap-profiler.h
    HINTS ${PC_GPERFTOOLS_INCLUDEDIR}
          ${PC_GPERFTOOLS_INCLUDE_DIRS}
		  $ENV{GPERFTOOLS_HOME}/include
		  $ENV{GPERFTOOLS_ROOT}/include
		  /usr/local/include
		  /usr/include
    PATH_SUFFIXES gperftools
)

FIND_LIBRARY(GPERFTOOLS_LIBRARIES
    NAMES tcmalloc libtcmalloc
    HINTS ${PC_GPERFTOOLS_LIBDIR}
          ${PC_GPERFTOOLS_LIBRARY_DIRS}
		  $ENV{GPERFTOOLS_HOME}/lib
		  $ENV{GPERFTOOLS_ROOT}/lib
		  /usr/local/lib
		  /usr/lib
		  /lib
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Gperftools DEFAULT_MSG GPERFTOOLS_INCLUDE_DIRS GPERFTOOLS_LIBRARIES)
