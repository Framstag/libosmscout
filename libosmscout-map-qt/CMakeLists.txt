set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(HEADER_FILES
	include/osmscout/MapQtImportExport.h
    include/osmscout/MapPainterQt.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterQt.cpp
)

osmscout_library_project(
	NAME OSMScoutMapQt
	ALIAS MapQt
	OUTPUT_NAME "osmscout_map_qt"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDES ${OPENGL_INCLUDE_DIR}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapQtFeatures.h.cmake
	TARGET OSMScout::OSMScout OSMScout::Map Qt5::Gui Qt5::Svg
)

if(APPLE AND BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutMapQt PROPERTIES
  		FRAMEWORK TRUE
  		FRAMEWORK_VERSION C
  		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  		PUBLIC_HEADER     "${HEADER_FILES}"
  		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutMapQt")
endif()
