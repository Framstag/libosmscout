# - Try to find libagg
# Once done, this will define
#
#  LIBAGG_FOUND - system has libagg
#  LIBAGG_INCLUDE_DIRS - the libagg include directories
#  LIBAGG_LIBRARIES - link these to use libagg
#  LIBAGGFT2_FOUND - system has aggfontfreetype
#
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_LIBAGG QUIET LIBAGG)

FIND_PATH(LIBAGG_INCLUDE_DIRS
    NAMES agg2/agg_basics.h
    HINTS ${PC_LIBAGG_INCLUDEDIR}
          ${PC_LIBAGG_INCLUDE_DIRS}
          $ENV{LIBAGG_HOME}/include
          $ENV{LIBAGG_ROOT}/include
          /usr/local/include
          /usr/include
          /libagg/include
    PATH_SUFFIXES agg agg2
)

FIND_LIBRARY(LIBAGG_LIBRARIES
    NAMES libagg agg libagg-2.5 agg-2.5
    HINTS ${PC_LIBAGG_LIBDIR}
          ${PC_LIBAGG_LIBRARY_DIRS}
          $ENV{LIBAGG_HOME}/lib
          $ENV{LIBAGG_ROOT}/lib
          /usr/local/lib
          /usr/lib
          /lib
          /libagg/lib
    PATH_SUFFIXES agg agg2
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBAGG DEFAULT_MSG LIBAGG_INCLUDE_DIRS LIBAGG_LIBRARIES)

IF(LIBAGG_FOUND AND FREETYPE_FOUND)
    PKG_CHECK_MODULES(PC_LIBAGGFT2 QUIET LIBAGGFT2)
    FIND_LIBRARY(LIBAGGFT2_LIBRARIES
        NAMES aggfontfreetype libaggfontfreetype
        HINTS ${PC_LIBAGG_LIBDIR}
              ${PC_LIBAGG_LIBRARY_DIRS}
              $ENV{LIBAGG_HOME}/lib
              $ENV{LIBAGG_ROOT}/lib
              /usr/local/lib
              /usr/lib
              /lib
              /libagg/lib
        PATH_SUFFIXES agg agg2
    )
    IF(LIBAGGFT2_LIBRARIES)
        SET(LIBAGG_LIBRARIES ${LIBAGG_LIBRARIES} ${LIBAGGFT2_LIBRARIES})
    ENDIF()

    MARK_AS_ADVANCED(LIBAGGFT2_LIBRARIES)
ENDIF()
