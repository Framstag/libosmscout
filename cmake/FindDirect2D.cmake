# - Find Direct2D in DirectX SDK installation
# Find the Direct2D includes and library
# This module defines
#  Direct2D_INCLUDE_DIRS, where to find d2d1.h, etc.
#  Direct2D_LIBRARIES, libraries to link against to use Direct2D.
#  Direct2D_FOUND, If false, do not try to use Direct2D.
#  DirectX_ROOT_DIR, directory where DirectX was installed.

set(sdk_include)
set(sdk_lib)
if(WIN32)
  get_filename_component(sdk_dir "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows;CurrentInstallFolder]" REALPATH)
  get_filename_component(kit_dir "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot]" REALPATH)
  get_filename_component(kit81_dir "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot81]" REALPATH)
  get_filename_component(kit10_dir "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" REALPATH)
  get_filename_component(kit10wow_dir "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" REALPATH)
  if(CMAKE_CL_64)
    set(subarch "x64")
    set(subarch2 "x64")
  else()
    set(subarch "x86")
    set(subarch2 "")
  endif()
  if(IS_DIRECTORY "${sdk_dir}/include" AND IS_DIRECTORY "${sdk_dir}/lib/${subarch2}")
    list(APPEND sdk_include "${sdk_dir}/include")
    list(APPEND sdk_lib "${sdk_dir}/lib/${subarch2}")
  endif()
  if(IS_DIRECTORY "${kit_dir}/include" AND IS_DIRECTORY "${kit_dir}/lib/${subarch2}")
    list(APPEND sdk_include "${kit_dir}/include")
    list(APPEND sdk_lib "${kit_dir}/lib/${subarch2}")
  endif()
  if(IS_DIRECTORY "${kit81_dir}/include/um" AND IS_DIRECTORY "${kit81_dir}/lib/${subarch}")
    list(APPEND sdk_include "${kit81_dir}/include/um")
    list(APPEND sdk_lib "${kit81_dir}/lib/${subarch}")
  endif()
  file(GLOB kit10_list ${kit10_dir}/Include/10.* ${kit10wow_dir}/Include/10.*)
  foreach(tmp_elem ${kit10_list})
    get_filename_component(tmp_ver ${tmp_elem} NAME)
    if(IS_DIRECTORY "${tmp_elem}/um" AND IS_DIRECTORY "${tmp_elem}/../../Lib/${tmp_ver}/um/${subarch}")
      list(APPEND sdk_include "${tmp_elem}/um")
      list(APPEND sdk_lib "${tmp_elem}/../../Lib/${tmp_ver}/um/${subarch}")
    endif()
  endforeach()
  list(REVERSE sdk_include)
  list(REVERSE sdk_lib)
endif()

FIND_PATH(Direct2D_INCLUDE_DIRS d2d1.h PATHS
	"${DirectX_ROOT_DIR}/Include"
	"/mingw64/x86_64-w64-mingw32/include"
	${sdk_include}
    "$ENV{DXSDK_DIR}/Include"
    "$ENV{PROGRAMFILES}/Microsoft DirectX SDK*/Include"
)

GET_FILENAME_COMPONENT(DirectX_ROOT_DIR "${DirectX_INCLUDE_DIRS}/.." ABSOLUTE)

SET(Direct2D_LIBRARY_PATHS
	"${DirectX_ROOT_DIR}/Lib/${subarch}"
	"/mingw64/x86_64-w64-mingw32/lib"
	${sdk_lib}
	"$ENV{DXSDK_DIR}/Lib/${subarch}/"
    "$ENV{PROGRAMFILES}/Microsoft DirectX SDK*/Lib/${subarch}/"
)

FIND_LIBRARY(Direct2D_D2D1_LIBRARY d2d1 ${Direct2D_LIBRARY_PATHS} NO_DEFAULT_PATH)
FIND_LIBRARY(Direct2D_DWRITE_LIBRARY dwrite ${Direct2D_LIBRARY_PATHS} NO_DEFAULT_PATH)
SET(Direct2D_LIBRARIES ${Direct2D_D2D1_LIBRARY} ${Direct2D_DWRITE_LIBRARY})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Direct2D DEFAULT_MSG DirectX_ROOT_DIR Direct2D_LIBRARIES Direct2D_INCLUDE_DIRS)
MARK_AS_ADVANCED(Direct2D_INCLUDE_DIRS Direct2D_D2D1_LIBRARY Direct2D_DWRITE_LIBRARY)
