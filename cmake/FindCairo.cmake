# - Try to find Cairo
# Once done, this will define
#
#  CAIRO_FOUND - system has Cairo
#  CAIRO_INCLUDE_DIRS - the Cairo include directories
#  CAIRO_LIBRARIES - link these to use Cairo
#
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_CAIRO QUIET cairo)

FIND_PATH(CAIRO_INCLUDE_DIRS
    NAMES cairo.h
    HINTS ${PC_CAIRO_INCLUDEDIR}
          ${PC_CAIRO_INCLUDE_DIRS}
		  $ENV{CAIRO_HOME}/include
		  $ENV{CAIRO_ROOT}/include
		  /usr/local/include
		  /usr/include
		  /cairo/include
    PATH_SUFFIXES cairo
)

FIND_LIBRARY(CAIRO_LIBRARIES
    NAMES cairo
    HINTS ${PC_CAIRO_LIBDIR}
          ${PC_CAIRO_LIBRARY_DIRS}
		  $ENV{CAIRO_HOME}/lib
		  $ENV{CAIRO_ROOT}/lib
		  /usr/local/lib
		  /usr/lib
		  /lib
		  /cairo/lib
)

IF (CAIRO_INCLUDE_DIRS)
    IF (EXISTS "${CAIRO_INCLUDE_DIRS}/cairo-version.h")
        FILE(READ "${CAIRO_INCLUDE_DIRS}/cairo-version.h" CAIRO_VERSION_CONTENT)
        STRING(REGEX MATCH "#define +CAIRO_VERSION_MAJOR +([0-9]+)" _dummy "${CAIRO_VERSION_CONTENT}")
        SET(CAIRO_VERSION_MAJOR "${CMAKE_MATCH_1}")
        STRING(REGEX MATCH "#define +CAIRO_VERSION_MINOR +([0-9]+)" _dummy "${CAIRO_VERSION_CONTENT}")
        SET(CAIRO_VERSION_MINOR "${CMAKE_MATCH_1}")
        STRING(REGEX MATCH "#define +CAIRO_VERSION_MICRO +([0-9]+)" _dummy "${CAIRO_VERSION_CONTENT}")
        SET(CAIRO_VERSION_MICRO "${CMAKE_MATCH_1}")
        SET(CAIRO_VERSION "${CAIRO_VERSION_MAJOR}.${CAIRO_VERSION_MINOR}.${CAIRO_VERSION_MICRO}")
    ENDIF ()
ENDIF ()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Cairo DEFAULT_MSG CAIRO_INCLUDE_DIRS CAIRO_LIBRARIES CAIRO_VERSION)
