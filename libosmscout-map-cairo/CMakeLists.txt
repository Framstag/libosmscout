if(NOT CAIRO_FOUND)
	message(SEND_ERROR "Cairo is required for cairo map drawing backend")
endif()
if(NOT PNG_FOUND)
	message(SEND_ERROR "libpng is required for cairo map drawing backend")
endif()
if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for cairo map drawing backend")
endif()

set(HEADER_FILES
    #include/osmscout/private/Config.h
    include/osmscout/private/MapCairoImportExport.h
    include/osmscout/LoaderPNG.h
    #include/osmscout/MapCairoFeatures.h
    include/osmscout/MapPainterCairo.h
)

set(SOURCE_FILES
    src/osmscout/LoaderPNG.cpp
    src/osmscout/MapPainterCairo.cpp
)

if(APPLE)
  set(THE_TARGET_NAME "OSMScoutMapCairo") 
else()
  set(THE_TARGET_NAME "osmscout_map_cairo") 
endif()

if(IOS)
  add_library(${THE_TARGET_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(${THE_TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_property(TARGET ${THE_TARGET_NAME} PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapCairoFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapCairoFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "${THE_TARGET_NAME}")
target_include_directories(${THE_TARGET_NAME} PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${CAIRO_INCLUDE_DIRS}
		${PNG_INCLUDE_DIRS})
if(APPLE)
target_link_libraries(${THE_TARGET_NAME}
		OSMScout
		OSMScoutMap
		${CAIRO_LIBRARIES}
		${PNG_LIBRARIES})
else()
target_link_libraries(${THE_TARGET_NAME}
		osmscout
		osmscout_map
		${CAIRO_LIBRARIES}
		${PNG_LIBRARIES})
endif()
if(PANGOCAIRO_FOUND)
  target_include_directories(${THE_TARGET_NAME} PRIVATE ${PANGO_INCLUDE_DIRS})
  target_link_libraries(${THE_TARGET_NAME} ${PANGO_LIBRARIES})
endif()

if(${IOS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
endif()

if(APPLE)
set_target_properties(${THE_TARGET_NAME} PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION C
  MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  PUBLIC_HEADER     "${HEADER_FILES}"
  CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
)
endif()

target_compile_definitions(${THE_TARGET_NAME} PRIVATE -DOSMSCOUT_MAP_CAIRO_EXPORT_SYMBOLS ${PNG_DEFINITIONS})
install(TARGETS ${THE_TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapCairoFeatures.h DESTINATION include/osmscout)
