# - Try to find Iconv
# Once done this will define
#
#  ICONV_FOUND - system has Iconv
#  ICONV_INCLUDE_DIR - the Iconv include directory
#  ICONV_LIBRARIES - Link these to use Iconv
#  ICONV_SECOND_ARGUMENT_IS_CONST - the second argument for iconv() is const
#
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CMakePushCheckState)

#if (ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
  # Already in cache, be silent
#  set(ICONV_FIND_QUIETLY TRUE)
#endif ()

if(APPLE)
    find_path(ICONV_INCLUDE_DIR iconv.h
             PATHS
             /opt/local/include/
             NO_CMAKE_SYSTEM_PATH
    )

    find_library(ICONV_LIBRARIES NAMES iconv libiconv c
             PATHS
             /opt/local/lib/
             NO_CMAKE_SYSTEM_PATH
    )
endif()

find_path(ICONV_INCLUDE_DIR iconv.h PATHS /opt/local/include /sw/include /usr/include /usr/local/include)

string(REGEX REPLACE "(.*)/include/?" "\\1" ICONV_INCLUDE_BASE_DIR "${ICONV_INCLUDE_DIR}")

find_library(ICONV_LIBRARIES NAMES libiconv iconv libiconv.lib libiconv.dylib)

if(NOT ICONV_LIBRARIES AND UNIX)
    find_library(ICONV_LIBRARIES NAMES c libc)
endif()

if(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
	cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIR})
	set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARIES})
    #check_prototype_definition("iconv"
    #        "size_t iconv(iconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)"
    #        "-1"
    #        "iconv.h"
    #        ICONV_SECOND_ARGUMENT_IS_CONST)
	check_cxx_source_compiles("
	  #include <iconv.h>
	  int main(){
		iconv_t conv = 0;
		const char* in = 0;
		size_t ilen = 0;
		char* out = 0;
		size_t olen = 0;
		iconv(conv, &in, &ilen, &out, &olen);
		return 0;
	  }
	" ICONV_SECOND_ARGUMENT_IS_CONST)
    set(CMAKE_REQUIRED_INCLUDES)
	set(CMAKE_REQUIRED_LIBRARIES)
	cmake_pop_check_state()
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ICONV DEFAULT_MSG ICONV_INCLUDE_DIR ICONV_LIBRARIES)

# Copy the results to the output variables.
if(ICONV_FOUND)
  set(ICONV_LIBRARY ${ICONV_LIBRARIES})
  set(ICONV_INCLUDE_DIRS ${ICONV_INCLUDE_DIR})
endif()

mark_as_advanced(
    ICONV_INCLUDE_DIR
    ICONV_LIBRARIES
    ICONV_SECOND_ARGUMENT_IS_CONST
)
