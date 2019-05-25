set(OSMSCOUT_BUILD_MAP ON CACHE INTERNAL "" FORCE)

set(HEADER_FILES
	include/osmscout/MapImportExport.h
	include/osmscout/oss/Parser.h
	include/osmscout/oss/Scanner.h
	include/osmscout/LabelLayouter.h
	include/osmscout/MapPainter.h
	include/osmscout/MapParameter.h
	include/osmscout/MapService.h
	include/osmscout/LabelProvider.h
	include/osmscout/LabelPath.h
	include/osmscout/Styles.h
	include/osmscout/StyleDescription.h
	include/osmscout/StyleConfig.h
	include/osmscout/StyleProcessor.h
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
	src/osmscout/LabelPath.cpp
	src/osmscout/Styles.cpp
	src/osmscout/StyleDescription.cpp
	src/osmscout/StyleConfig.cpp
	src/osmscout/StyleProcessor.cpp
	src/osmscout/DataTileCache.cpp
	src/osmscout/MapTileCache.cpp
	src/osmscout/MapPainterNoOp.cpp
)

if(IOS)
  add_library(OSMScoutMap STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(OSMScoutMap ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_target_properties(OSMScoutMap PROPERTIES
		CXX_STANDARD 14
		OUTPUT_NAME "osmscout_map")

target_include_directories(OSMScoutMap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Public features Header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapFeatures.h)
target_include_directories(OSMScoutMap PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)

# Private config header
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutMap")
target_include_directories(OSMScoutMap PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/privateinclude)

target_link_libraries(OSMScoutMap OSMScout)

if(${IOS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
endif()

if(APPLE AND BUILD_FRAMEWORKS)
set_target_properties(OSMScoutMap PROPERTIES
  		FRAMEWORK TRUE
  		FRAMEWORK_VERSION C
  		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  		PUBLIC_HEADER     "${HEADER_FILES}"
  		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutMap")
endif()

target_compile_definitions(OSMScoutMap PRIVATE -DOSMSCOUT_MAP_EXPORT_SYMBOLS)

install(TARGETS OSMScoutMap
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapFeatures.h DESTINATION include/osmscout)
