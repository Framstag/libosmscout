if(NOT LIBAGG_FOUND)
	message(SEND_ERROR "libagg is required for agg map drawing backend")
endif()
if(NOT LIBAGGFT2_LIBRARIES)
	message(SEND_ERROR "libagg is required for agg map drawing backend")
endif()
if(NOT FREETYPE_FOUND)
	message(SEND_ERROR "freetype2 is required for agg map drawing backend")
endif()
if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for agg map drawing backend")
endif()

set(HEADER_FILES
    #include/osmscout/private/Config.h
    include/osmscout/private/MapAggImportExport.h
    include/osmscout/MapAggFeatures.h
    include/osmscout/MapPainterAgg.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterAgg.cpp
)

if(APPLE AND BUILD_FRAMEWORKS)
  set(THE_TARGET_NAME "OSMScoutMapAGG") 
else()
  set(THE_TARGET_NAME "osmscout_map_agg") 
endif()

if(IOS)
  add_library(${THE_TARGET_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(${THE_TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_property(TARGET ${THE_TARGET_NAME} PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapAggFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapAggFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "${THE_TARGET_NAME}")
target_include_directories(${THE_TARGET_NAME} PRIVATE
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${LIBAGG_INCLUDE_DIRS}
		${FREETYPE_INCLUDE_DIRS})
if(APPLE AND BUILD_FRAMEWORKS)
target_link_libraries(${THE_TARGET_NAME}
		OSMScout
		OSMScoutMap
		${LIBAGG_LIBRARIES}
		${LIBAGGFT2_LIBRARIES}
		${FREETYPE_LIBRARIES})
else()
target_link_libraries(${THE_TARGET_NAME}
		osmscout
		osmscout_map
		${LIBAGG_LIBRARIES}
		${LIBAGGFT2_LIBRARIES}
		${FREETYPE_LIBRARIES})
endif()

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

target_compile_definitions(${THE_TARGET_NAME} PRIVATE -DOSMSCOUT_MAP_AGG_EXPORT_SYMBOLS)
install(TARGETS ${THE_TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapAggFeatures.h DESTINATION include/osmscout)
