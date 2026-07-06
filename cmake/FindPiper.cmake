# - Try to find Piper TTS (libpiper)
#
# Upstream libpiper ships neither a CMake package config nor a pkg-config file,
# so this module locates the header and library manually. A pkg-config lookup is
# still attempted first as a fallback for future versions / custom builds.
#
# The following hints are honoured (in addition to the standard system paths):
#   PIPER_ROOT (CMake variable)
#   /opt/piper
#
# Once done, this will define:
#   Piper_FOUND         - system has libpiper
#   Piper_INCLUDE_DIRS  - the libpiper include directories
#   Piper_LIBRARIES     - link these to use libpiper
#
# and the imported target:
#   Piper::Piper

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(PC_PIPER QUIET piper libpiper)
endif()

find_path(Piper_INCLUDE_DIRS
    NAMES piper.h
    HINTS ${PC_PIPER_INCLUDEDIR}
          ${PC_PIPER_INCLUDE_DIRS}
          ${PIPER_ROOT}
          /opt/piper
    PATH_SUFFIXES include piper
)

find_library(Piper_LIBRARIES
    NAMES piper libpiper
    HINTS ${PC_PIPER_LIBDIR}
          ${PC_PIPER_LIBRARY_DIRS}
          ${PIPER_ROOT}
          /opt/piper
    PATH_SUFFIXES lib lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Piper
    REQUIRED_VARS Piper_LIBRARIES Piper_INCLUDE_DIRS)

if(Piper_FOUND AND NOT TARGET Piper::Piper)
  add_library(Piper::Piper UNKNOWN IMPORTED)
  set_target_properties(Piper::Piper PROPERTIES
      IMPORTED_LOCATION "${Piper_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${Piper_INCLUDE_DIRS}")
endif()

mark_as_advanced(Piper_INCLUDE_DIRS Piper_LIBRARIES)

