# - Find Direct2D in DirectX SDK installation
# Find the Direct2D includes and library
# This module defines
#  Direct2D_INCLUDE_DIRS, where to find d2d1.h, etc.
#  Direct2D_LIBRARIES, libraries to link against to use Direct2D.
#  Direct2D_FOUND, If false, do not try to use Direct2D.
#  DirectX_ROOT_DIR, directory where DirectX was installed.

FIND_PATH(Direct2D_INCLUDE_DIRS d2d1.h PATHS
    "$ENV{DXSDK_DIR}/Include"
    "$ENV{PROGRAMFILES}/Microsoft DirectX SDK*/Include"
    "$ENV{PROGRAMFILES}/Microsoft SDKs/Windows/*/Include"
	"C:/Program Files (x86)/Windows Kits/10/include/10.0.14393.0/um"
	"C:/Program Files/Windows Kits/10/include/10.0.14393.0/um"
	"C:/Program Files (x86)/Windows Kits/10/include/10.0.10586.0/um"
	"C:/Program Files/Windows Kits/10/include/10.0.10586.0/um"
)

GET_FILENAME_COMPONENT(DirectX_ROOT_DIR "${DirectX_INCLUDE_DIRS}/.." ABSOLUTE)

IF (CMAKE_CL_64)
    SET(Direct2D_LIBRARY_PATHS "${DirectX_ROOT_DIR}/Lib/x64" "$ENV{DXSDK_DIR}/Lib/x64/" "C:/Program Files (x86)/Windows Kits/10/lib/10.0.14393.0/um/x64/" "C:/Program Files (x86)/Windows Kits/10/lib/10.0.10586.0/um/x64/")
ELSE ()
    SET(Direct2D_LIBRARY_PATHS "${DirectX_ROOT_DIR}/Lib/x86" "$ENV{DXSDK_DIR}/Lib/x86/" "C:/Program Files (x86)/Windows Kits/10/lib/10.0.14393.0/um/x86/" "C:/Program Files (x86)/Windows Kits/10/lib/10.0.10586.0/um/x86/" "${DirectX_ROOT_DIR}/Lib")
ENDIF ()

FIND_LIBRARY(Direct2D_D2D1_LIBRARY d2d1 ${Direct2D_LIBRARY_PATHS} NO_DEFAULT_PATH)
FIND_LIBRARY(Direct2D_DWRITE_LIBRARY dwrite ${Direct2D_LIBRARY_PATHS} NO_DEFAULT_PATH)
SET(Direct2D_LIBRARIES ${Direct2D_D2D1_LIBRARY} ${Direct2D_DWRITE_LIBRARY})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Direct2D DEFAULT_MSG DirectX_ROOT_DIR Direct2D_LIBRARIES Direct2D_INCLUDE_DIRS)
MARK_AS_ADVANCED(Direct2D_INCLUDE_DIRS Direct2D_D2D1_LIBRARY Direct2D_DWRITE_LIBRARY)
