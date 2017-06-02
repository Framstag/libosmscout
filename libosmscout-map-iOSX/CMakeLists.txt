if(NOT APPLE)
	message(SEND_ERROR "Mac OS X or iOS required for iOSX map drawing backend")
endif()

if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for cairo map drawing backend")
endif()

set(HEADER_FILES
    #include/osmscout/private/Config.h
    include/osmscout/private/Math.h
    #include/osmscout/MapCairoFeatures.h
    include/osmscout/MapPainterIOS.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterIOS.mm
)

add_library(osmscout_map_iosx ${SOURCE_FILES} ${HEADER_FILES})

set_property(TARGET osmscout_map_iosx PROPERTY CXX_STANDARD 11)

create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "osmscout-map-iosx")

find_library(FOUNDATION Foundation)
find_library(COCOA Cocoa)
find_library(APPKIT AppKit)
find_library(COREDATA CoreData)

target_include_directories(osmscout_map_iosx PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)

target_link_libraries(osmscout_map_iosx
		osmscout
		osmscout_map
		${FOUNDATION}
		${COCOA}
		${APPKIT}
		${COREDATA})
		
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-arc")
				
target_compile_definitions(osmscout_map_iosx PRIVATE -DOSMSCOUT_MAP_IOSX_EXPORT_SYMBOLS)
install(TARGETS osmscout_map_iosx
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
#install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapCairoFeatures.h DESTINATION include/osmscout)
