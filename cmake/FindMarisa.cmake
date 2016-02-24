# - Try to find Marisa
# Once done, this will define
#
#  MARISA_FOUND - system has MARISA
#  MARISA_INCLUDE_DIRS - the MARISA include directories
#  MARISA_LIBRARIES - link these to use MARISA
#
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_MARISA QUIET MARISA)

FIND_PATH(MARISA_INCLUDE_DIRS
    NAMES marisa.h
    HINTS ${PC_MARISA_INCLUDEDIR}
          ${PC_MARISA_INCLUDE_DIRS}
		  $ENV{MARISA_HOME}/include
		  $ENV{MARISA_ROOT}/include
		  /usr/local/include
		  /usr/include
		  /marisa/include
    PATH_SUFFIXES marisa
)

FIND_LIBRARY(MARISA_LIBRARIES
    NAMES marisa libmarisa
    HINTS ${PC_MARISA_LIBDIR}
          ${PC_MARISA_LIBRARY_DIRS}
		  $ENV{MARISA_HOME}/lib
		  $ENV{MARISA_ROOT}/lib
		  /usr/local/lib
		  /usr/lib
		  /lib
		  /marisa/lib
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MARISA DEFAULT_MSG MARISA_INCLUDE_DIRS MARISA_LIBRARIES)
