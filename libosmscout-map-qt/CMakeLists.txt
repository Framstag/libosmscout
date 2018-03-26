if(NOT Qt5Gui_FOUND)
	message(SEND_ERROR "Qt5 is required for Qt map drawing backend")
endif()
if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for Qt map drawing backend")
endif()

set(HEADER_FILES
    #include/osmscout/private/Config.h
    include/osmscout/private/MapQtImportExport.h
    include/osmscout/SimplifiedPath.h
    include/osmscout/MapPainterQt.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterQt.cpp
    src/osmscout/SimplifiedPath.cpp
)

if(APPLE AND BUILD_FRAMEWORKS)
  set(THE_TARGET_NAME "OSMScoutMapQt") 
else()
  set(THE_TARGET_NAME "osmscout_map_qt") 
endif()

if(IOS)
  add_library(${THE_TARGET_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(${THE_TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_property(TARGET ${THE_TARGET_NAME} PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapQtFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapQtFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "${THE_TARGET_NAME}")
target_include_directories(${THE_TARGET_NAME} PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${Qt5Gui_INCLUDE_DIRS})

if(${IOS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
endif()

if(APPLE AND BUILD_FRAMEWORKS)
set_target_properties(${THE_TARGET_NAME} PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION C
  MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  PUBLIC_HEADER     "${HEADER_FILES}"
  CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
)
endif()

if(APPLE AND BUILD_FRAMEWORKS)
target_link_libraries(${THE_TARGET_NAME}
		OSMScout
		OSMScoutMap
		${Qt5Gui_LIBRARIES})
else()
target_link_libraries(${THE_TARGET_NAME}
		osmscout
		osmscout_map
		${Qt5Gui_LIBRARIES})
endif()
target_compile_definitions(${THE_TARGET_NAME} PRIVATE -DOSMSCOUT_MAP_QT_EXPORT_SYMBOLS ${Qt5Gui_DEFINITIONS})
install(TARGETS ${THE_TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapQtFeatures.h DESTINATION include/osmscout)
