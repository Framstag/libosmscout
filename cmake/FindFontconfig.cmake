# - try to find FONTCONFIG
#
#  FONTCONFIG_FOUND - system has Cairo
#  FONTCONFIG_CFLAGS - the Cairo CFlags
#  FONTCONFIG_INCLUDE_DIRS - the Cairo include directories
#  FONTCONFIG_LIBRARIES - Link these to use Cairo
#
# Copyright (C) 2017 Tim Teulings
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(FONTCONFIG_INCLUDE_DIRS AND FONTCONFIG_LIBRARIES)

  # in cache already
  set(FONTCONFIG_FOUND TRUE)

else(FONTCONFIG_INCLUDE_DIRS AND FONTCONFIG_LIBRARIES)

if(NOT WIN32)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(_pc_fontconfig fontconfig)
  if(_pc_fontconfig_FOUND)
    set(FONTCONFIG_FOUND TRUE)
  endif(_pc_fontconfig_FOUND)
else(NOT WIN32)
  # assume so, for now
  set(FONTCONFIG_FOUND TRUE)
endif(NOT WIN32)

if(FONTCONFIG_FOUND)
  # set it back as false
  set(FONTCONFIG_FOUND FALSE)

  find_library(FONTCONFIG_LIBRARY fontconfig
               HINTS ${_pc_fontconfig_LIBRARY_DIRS}
  )
  set(FONTCONFIG_LIBRARIES "${FONTCONFIG_LIBRARY}")

  find_path(FONTCONFIG_INCLUDE_DIR fontconfig/fontconfig.h
            HINTS ${_pc_fontconfig_INCLUDE_DIRS}
            PATH_SUFFIXES fontconfig
  )
  set(FONTCONFIG_INCLUDE_DIRS "${FONTCONFIG_INCLUDE_DIR}")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Fontconfig DEFAULT_MSG FONTCONFIG_LIBRARIES FONTCONFIG_INCLUDE_DIRS)
endif(FONTCONFIG_FOUND)

endif(FONTCONFIG_INCLUDE_DIRS AND FONTCONFIG_LIBRARIES)

mark_as_advanced(
  FONTCONFIG_CFLAGS
  FONTCONFIG_INCLUDE_DIRS
  FONTCONFIG_LIBRARIES
)
