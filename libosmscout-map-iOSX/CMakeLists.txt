set(HEADER_FILES
        include/osmscout/MapPainterIOS.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterIOS.mm
)

add_library(OSMScoutMapIOSX ${SOURCE_FILES} ${HEADER_FILES})
add_library(OSMScout::MapIOSX ALIAS OSMScoutMapIOSX)
set_target_properties(OSMScoutMapIOSX PROPERTIES
	OUTPUT_NAME "osmscout_map_iosx"
	VERSION ${OSMSCOUT_LIBRARY_VERSION}
	SOVERSION ${OSMSCOUT_LIBRARY_VERSION}
	FOLDER "libosmscout"
)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutMapIOSX")
target_include_directories(OSMScoutMapIOSX PRIVATE
	include
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/privateinclude
	${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(OSMScoutMapIOSX INTERFACE ${CMAKE_SOURCE_DIR}/libosmscout-map-iOSX/include ${CMAKE_BINARY_DIR}/libosmscout-map-iOSX/include)
target_link_libraries(OSMScoutMapIOSX OSMScout::OSMScout OSMScout::Map)

set_target_properties(OSMScoutMapIOSX PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION A
  MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  PUBLIC_HEADER     include/osmscout/MapPainterIOS.h
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

install(TARGETS OSMScoutMapIOSX
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapCairoFeatures.h DESTINATION include/osmscout)
