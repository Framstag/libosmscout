set(OSMSCOUT_BUILD_MAP ON CACHE INTERNAL "" FORCE)

set(HEADER_FILES
	include/osmscout/oss/Parser.h
	include/osmscout/oss/Scanner.h
	#include/osmscout/private/Config.h
	#include/osmscout/private/MapImportExport.h
	include/osmscout/MapFeatures.h
	include/osmscout/LabelLayouter.h
	include/osmscout/MapPainter.h
	include/osmscout/MapParameter.h
	include/osmscout/MapService.h
	include/osmscout/LabelProvider.h
	include/osmscout/Styles.h
	include/osmscout/StyleDescription.h
	include/osmscout/StyleConfig.h
	include/osmscout/StyleProcessor.h
	include/osmscout/TileId.h
	include/osmscout/DataTileCache.h
	include/osmscout/MapTileCache.h
	include/osmscout/MapPainterNoOp.h
)

set(SOURCE_FILES
	src/osmscout/oss/Parser.cpp
	src/osmscout/oss/Scanner.cpp
	src/osmscout/LabelLayouter.cpp
	src/osmscout/MapPainter.cpp
	src/osmscout/MapParameter.cpp
	src/osmscout/MapService.cpp
	src/osmscout/LabelProvider.cpp
	src/osmscout/Styles.cpp
	src/osmscout/StyleDescription.cpp
	src/osmscout/StyleConfig.cpp
	src/osmscout/StyleProcessor.cpp
	src/osmscout/TileId.cpp
	src/osmscout/DataTileCache.cpp
	src/osmscout/MapTileCache.cpp
	src/osmscout/MapPainterNoOp.cpp
)

if(APPLE AND BUILD_FRAMEWORKS)
set(THE_TARGET_NAME "OSMScoutMap")
else()
set(THE_TARGET_NAME "osmscout_map")
endif()

if(IOS)
  add_library(${THE_TARGET_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(${THE_TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_property(TARGET ${THE_TARGET_NAME} PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "osmscout-map")
target_include_directories(${THE_TARGET_NAME} PRIVATE
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
if(APPLE AND BUILD_FRAMEWORKS)
  target_link_libraries(${THE_TARGET_NAME} OSMScout)
else()
  target_link_libraries(${THE_TARGET_NAME} osmscout)
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

target_compile_definitions(${THE_TARGET_NAME} PRIVATE -DOSMSCOUT_MAP_EXPORT_SYMBOLS)
install(TARGETS ${THE_TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapFeatures.h DESTINATION include/osmscout)
