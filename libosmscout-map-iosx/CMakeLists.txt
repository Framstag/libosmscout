set(HEADER_FILES
    include/osmscoutmapiosx/MapPainterIOS.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterIOS.mm
)

osmscout_library_project(
	NAME OSMScoutMapIOSX
	ALIAS MapIOSX
	OUTPUT_NAME "osmscout_map_iosx"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDEDIR "osmscoutmapiosx"
	TARGET OSMScout::OSMScout OSMScout::Map
)

set_target_properties(OSMScoutMapIOSX PROPERTIES
  UNITY_BUILD OFF
  FRAMEWORK TRUE
  FRAMEWORK_VERSION A
  MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  PUBLIC_HEADER  "${HEADER_FILES}"
  CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
  OUTPUT_NAME "OSMScoutMapIOSX")

find_library(FOUNDATION Foundation)
find_library(COREGRAPHICS CoreGraphics)
find_library(CORETEXT CoreText)

if(${IOS})
	find_library(UIKIT UIKit)
	target_link_libraries(OSMScoutMapIOSX
		${FOUNDATION}
		${UIKIT}
		${CORETEXT}
		${COREGRAPHICS})
else()
	find_library(COCOA Cocoa)
	find_library(APPKIT AppKit)
	target_link_libraries(OSMScoutMapIOSX
		${FOUNDATION}
		${COCOA}
		${APPKIT}
		${CORETEXT}
		${COREGRAPHICS})
endif()
