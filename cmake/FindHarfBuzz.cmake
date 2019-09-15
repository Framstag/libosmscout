# - Try to find HarfBuzz
# Once done, this will define
#
#  HARFBUZZ_FOUND - system has HARFBUZZ
#  HARFBUZZ_INCLUDE_DIRS - the HARFBUZZ include directories
#  HARFBUZZ_LIBRARIES - link these to use HARFBUZZ
#
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_HARFBUZZ QUIET HARFBUZZ)

FIND_PATH(HARFBUZZ_INCLUDE_DIRS
        NAMES hb.h
        HINTS ${PC_HARFBUZZ_INCLUDEDIR}
        ${PC_HARFBUZZ_INCLUDE_DIRS}
        $ENV{HARFBUZZ_HOME}/include
        $ENV{HARFBUZZ_ROOT}/include
        /usr/local/include
        /usr/include
        /harfbuzz/include
        PATH_SUFFIXES harfbuzz
        )

FIND_LIBRARY(HARFBUZZ_LIBRARIES
        NAMES harfbuzz libmarisa
        HINTS ${PC_HARFBUZZ_LIBDIR}
        ${PC_HARFBUZZ_LIBRARY_DIRS}
        $ENV{HARFBUZZ_HOME}/lib
        $ENV{HARFBUZZ_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /lib
        /harfbuzz/lib
        )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HARFBUZZ DEFAULT_MSG HARFBUZZ_INCLUDE_DIRS HARFBUZZ_LIBRARIES)
