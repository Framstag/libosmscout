# - Try to find Pango
# Once done, this will define
#
#  PANGO_FOUND - system has Pango
#  PANGO_INCLUDE_DIRS - the Pango include directories
#  PANGO_LIBRARIES - link these to use Pango

find_package(PkgConfig)
pkg_check_modules(PC_PANGO QUIET pango)
set(PANGO_DEFINITIONS ${PC_PANGO_CFLAGS_OTHER})
find_path(PANGO_INCLUDE_DIR pango/pango.h HINTS ${PC_PANGO_INCLUDE_DIR} ${PC_PANGO_INCLUDE_DIRS} PATH_SUFFIXES pango PANGO PANGO-1.0 pango-1.0 )
find_library(PANGO_LIBRARY NAMES pango PANGO PANGO-1.0 pango-1.0 HINTS ${PC_PANGO_LIBDIR} ${PC_PANGO_LIBRARY_DIRS})
find_library(PANGOCAIRO_LIBRARY NAMES pangocairo PANGOcairo PANGOcairo-1.0 pangocairo-1.0 HINTS ${PC_PANGO_LIBDIR} ${PC_PANGO_LIBRARY_DIRS})
find_library(PANGOFT2_LIBRARY NAMES pangoft2 PANGOft2 PANGOft2-1.0 pangoft2-1.0 HINTS ${PC_PANGO_LIBDIR} ${PC_PANGO_LIBRARY_DIRS})
set(PANGO_LIBRARIES ${PANGO_LIBRARY} ${PANGOCAIRO_LIBRARY} ${PANGOFT2_LIBRARY})
set(PANGO_INCLUDE_DIRS ${PANGO_INCLUDE_DIR})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PANGO DEFAULT_MSG PANGO_LIBRARY PANGO_INCLUDE_DIR)
mark_as_advanced(PANGO_INCLUDE_DIR PANGO_LIBRARY PANGOCAIRO_LIBRARY PANGOFT2_LIBRARY)
