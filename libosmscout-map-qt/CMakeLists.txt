set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(HEADER_FILES
	include/osmscout/MapQtImportExport.h
    include/osmscout/MapPainterQt.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterQt.cpp
)

add_library(OSMScoutMapQt ${SOURCE_FILES} ${HEADER_FILES})
add_library(OSMScout::MapQt ALIAS OSMScoutMapQt)
set_target_properties(OSMScoutMapQt PROPERTIES
	OUTPUT_NAME "osmscout_map_qt"
	VERSION ${OSMSCOUT_LIBRARY_VERSION}
	SOVERSION ${OSMSCOUT_LIBRARY_VERSION}
	FOLDER "libosmscout"
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapQtFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapQtFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutMapQt")
target_include_directories(OSMScoutMapQt PRIVATE
	include
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/privateinclude
	${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(OSMScoutMapQt INTERFACE ${CMAKE_SOURCE_DIR}/libosmscout-map-qt/include ${CMAKE_BINARY_DIR}/libosmscout-map-qt/include)
target_link_libraries(OSMScoutMapQt OSMScout::OSMScout OSMScout::Map Qt5::Gui Qt5::Svg)

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

if(WIN32 AND BUILD_SHARED_LIBS)
	create_win32_dll_resource(OSMScoutMapQt "osmscout_map_qt")
endif()
if(OSMSCOUT_INSTALL_QT_DLL AND TARGET Qt5::windeployqt AND BUILD_SHARED_LIBS)
	add_custom_command(TARGET OSMScoutMapQt
        POST_BUILD
        COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
        COMMAND Qt5::windeployqt --dir "${CMAKE_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:OSMScoutMapQt>/$<TARGET_FILE_NAME:OSMScoutMapQt>"
    )
endif()

install(TARGETS OSMScoutMapQt
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapQtFeatures.h DESTINATION include/osmscout)
