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
    include/osmscout/MapAggImportExport.h
    include/osmscout/MapAggFeatures.h
    include/osmscout/MapPainterAgg.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterAgg.cpp
)

if(IOS)
  add_library(OSMScoutMapAGG STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(OSMScoutMapAGG ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_target_properties(OSMScoutMapAGG PROPERTIES
		CXX_STANDARD 14
		OUTPUT_NAME "osmscout_map_agg")

target_include_directories(OSMScoutMapAGG PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${LIBAGG_INCLUDE_DIRS}
		${FREETYPE_INCLUDE_DIRS})

# Public features Header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapAggFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapAggFeatures.h)
target_include_directories(OSMScoutMapAGG PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)

# Private config header
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutMapAGG")
target_include_directories(OSMScoutMapAGG PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/privateinclude)

target_link_libraries(OSMScoutMapAGG
		OSMScout
		OSMScoutMap
		${LIBAGG_LIBRARIES}
		${LIBAGGFT2_LIBRARIES}
		${FREETYPE_LIBRARIES})

if(${IOS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
endif()

if(APPLE AND BUILD_FRAMEWORKS)
set_target_properties(OSMScoutMapAGG PROPERTIES
  		FRAMEWORK TRUE
  		FRAMEWORK_VERSION C
  		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  		PUBLIC_HEADER     "${HEADER_FILES}"
  		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutMapAGG")
endif()

target_compile_definitions(OSMScoutMapAGG PRIVATE -DOSMSCOUT_MAP_AGG_EXPORT_SYMBOLS)

install(TARGETS OSMScoutMapAGG
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapAggFeatures.h DESTINATION include/osmscout)
