# - Try to find Pango
# Once done, this will define
#
#  PANGO_FOUND - system has Pango
#  PANGO_INCLUDE_DIRS - the Pango include directories
#  PANGO_LIBRARIES - link these to use Pango
#  PANGO_DEFINITIONS - compiler flags

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_PANGO pango) # FIXME: After we require CMake 2.8.2 we can pass QUIET to this call.
SET(PANGO_DEFINITIONS ${PC_PANGO_CFLAGS_OTHER})

FIND_PATH(PANGO_INCLUDE_DIR
    NAMES pango/pango.h
    HINTS ${PC_PANGO_INCLUDEDIR}
          ${PC_PANGO_INCLUDE_DIRS}
          $ENV{PANGO_HOME}/include
          $ENV{PANGO_ROOT}/include
          /usr/local/include
          /usr/include
          /pango/include
    PATH_SUFFIXES pango pango-1.0 pango1.0
)

FIND_LIBRARY(PANGO_LIBRARY
    NAMES pango PANGO PANGO-1.0 pango-1.0
    HINTS ${PC_PANGO_LIBDIR}
          ${PC_PANGO_LIBRARY_DIRS}
          $ENV{PANGO_HOME}/lib
          $ENV{PANGO_ROOT}/lib
          /usr/local/lib
          /usr/lib
          /lib
          /pango/lib
    PATH_SUFFIXES pango pango-1.0 pango1.0
)

FIND_LIBRARY(PANGOCAIRO_LIBRARIES
    NAMES pangocairo PANGOcairo PANGOcairo-1.0 pangocairo-1.0
    HINTS ${PC_PANGO_LIBDIR}
          ${PC_PANGO_LIBRARY_DIRS}
          $ENV{PANGO_HOME}/lib
          $ENV{PANGO_ROOT}/lib
          /usr/local/lib
          /usr/lib
          /lib
          /pango/lib
    PATH_SUFFIXES pango pango-1.0 pango1.0
)

FIND_LIBRARY(PANGOFT2_LIBRARIES
    NAMES pangoft2 PANGOft2 PANGOft2-1.0 pangoft2-1.0
    HINTS ${PC_PANGO_LIBDIR}
          ${PC_PANGO_LIBRARY_DIRS}
          $ENV{PANGO_HOME}/lib
          $ENV{PANGO_ROOT}/lib
          /usr/local/lib
          /usr/lib
          /lib
          /pango/lib
    PATH_SUFFIXES pango pango-1.0 pango1.0
)

SET(PANGO_LIBRARIES ${PANGO_LIBRARY} ${PANGOCAIRO_LIBRARY} ${PANGOFT2_LIBRARY})
SET(PANGO_INCLUDE_DIRS ${PANGO_INCLUDE_DIR})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Pango DEFAULT_MSG PANGO_INCLUDE_DIRS PANGO_LIBRARIES)
MARK_AS_ADVANCED(PANGO_INCLUDE_DIR PANGO_LIBRARY PANGOCAIRO_LIBRARY PANGOFT2_LIBRARY)
