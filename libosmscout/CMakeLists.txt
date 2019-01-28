set(OSMSCOUT_BUILD_CORE ON CACHE INTERNAL "" FORCE)

set(HEADER_FILES_OST
    include/osmscout/ost/Parser.h
    include/osmscout/ost/Scanner.h)

set(HEADER_FILES_SYSTEM
    include/osmscout/system/Assert.h
    include/osmscout/system/Compiler.h
    include/osmscout/system/Math.h
    include/osmscout/system/SSEMath.h
    include/osmscout/system/SSEMathPublic.h
    include/osmscout/system/OSMScoutTypes.h)

set(HEADER_FILES_UTIL
    include/osmscout/util/Base64.h
    include/osmscout/util/Breaker.h
    include/osmscout/util/Cache.h
    include/osmscout/util/Color.h
    include/osmscout/util/CmdLineParsing.h
    include/osmscout/util/Distance.h
    include/osmscout/util/Exception.h
    include/osmscout/util/File.h
    include/osmscout/util/FileScanner.h
    include/osmscout/util/FileWriter.h
    include/osmscout/util/HTMLWriter.h
    include/osmscout/util/GeoBox.h
    include/osmscout/util/Geometry.h
    include/osmscout/util/Logger.h
    include/osmscout/util/Magnification.h
    include/osmscout/util/MemoryMonitor.h
    include/osmscout/util/NodeUseMap.h
    include/osmscout/util/Number.h
    include/osmscout/util/NumberSet.h
    include/osmscout/util/Parsing.h
    include/osmscout/util/Progress.h
    include/osmscout/util/Projection.h
    include/osmscout/util/StopClock.h
    include/osmscout/util/String.h
    include/osmscout/util/StringMatcher.h
    include/osmscout/util/TagErrorReporter.h
    include/osmscout/util/Tiling.h
    include/osmscout/util/Time.h
    include/osmscout/util/TileId.h
    include/osmscout/util/Transformation.h
    include/osmscout/util/WorkQueue.h)

set(HEADER_FILES_ROUTING
    include/osmscout/routing/Route.h
    include/osmscout/routing/RouteData.h
    include/osmscout/routing/RouteNode.h
    include/osmscout/routing/RouteNodeDataFile.h
    include/osmscout/routing/RoutePostprocessor.h
    include/osmscout/routing/RoutingDB.h
    include/osmscout/routing/RoutingProfile.h
    include/osmscout/routing/RoutingService.h
    include/osmscout/routing/AbstractRoutingService.h
    include/osmscout/routing/SimpleRoutingService.h
    include/osmscout/routing/MultiDBRoutingService.h
    include/osmscout/routing/DBFileOffset.h
    include/osmscout/routing/TurnRestriction.h
    include/osmscout/routing/MultiDBRoutingState.h)

set(HEADER_FILES_NAVIGATION
    include/osmscout/navigation/Agents.h
    include/osmscout/navigation/DataAgent.h
    include/osmscout/navigation/PositionAgent.h
    include/osmscout/navigation/RouteStateAgent.h
    include/osmscout/navigation/Engine.h
    include/osmscout/navigation/Navigation.h)

set(HEADER_FILES
    ${HEADER_FILES_OST}
    ${HEADER_FILES_SYSTEM}
    ${HEADER_FILES_UTIL}
    ${HEADER_FILES_ROUTING}
    ${HEADER_FILES_NAVIGATION}
    include/osmscout/CoreImportExport.h
    include/osmscout/Area.h
    include/osmscout/AreaAreaIndex.h
    include/osmscout/AreaDataFile.h
    include/osmscout/AreaNodeIndex.h
    include/osmscout/AreaWayIndex.h
    include/osmscout/Coord.h
    include/osmscout/CoordDataFile.h
    include/osmscout/CoverageIndex.h
    include/osmscout/BoundingBoxDataFile.h
    include/osmscout/TypeDistributionDataFile.h
    include/osmscout/Database.h
    include/osmscout/DataFile.h
    include/osmscout/BasemapDatabase.h
    include/osmscout/DebugDatabase.h
    include/osmscout/GeoCoord.h
    include/osmscout/GroundTile.h
    include/osmscout/Intersection.h
    include/osmscout/Location.h
    include/osmscout/LocationIndex.h
    include/osmscout/LocationService.h
    include/osmscout/LocationDescriptionService.h
    include/osmscout/Node.h
    include/osmscout/NodeDataFile.h
    include/osmscout/NumericIndex.h
    include/osmscout/ObjectRef.h
    include/osmscout/OptimizeAreasLowZoom.h
    include/osmscout/OptimizeWaysLowZoom.h
    include/osmscout/Path.h
    include/osmscout/Pixel.h
    include/osmscout/Point.h
    include/osmscout/POIService.h
    include/osmscout/ObjectVariantDataFile.h
    include/osmscout/SRTM.h
    include/osmscout/Tag.h
    include/osmscout/TypeConfig.h
    include/osmscout/TypeFeature.h
    include/osmscout/TypeFeatures.h
    include/osmscout/TypeInfoSet.h
    include/osmscout/FeatureReader.h
    include/osmscout/OSMScoutTypes.h
    include/osmscout/WaterIndex.h
    include/osmscout/Way.h
    include/osmscout/WayDataFile.h)

set(SOURCE_FILES
    src/osmscout/ost/Parser.cpp
    src/osmscout/ost/Scanner.cpp
    src/osmscout/system/SSEMath.cpp
    src/osmscout/util/Breaker.cpp
    src/osmscout/util/Cache.cpp
    src/osmscout/util/Color.cpp
    src/osmscout/util/Distance.cpp
    src/osmscout/util/Exception.cpp
    src/osmscout/util/File.cpp
    src/osmscout/util/FileScanner.cpp
    src/osmscout/util/FileWriter.cpp
    src/osmscout/util/HTMLWriter.cpp
    src/osmscout/util/GeoBox.cpp
    src/osmscout/util/Geometry.cpp
    src/osmscout/util/Logger.cpp
    src/osmscout/util/Magnification.cpp
    src/osmscout/util/MemoryMonitor.cpp
    src/osmscout/util/NodeUseMap.cpp
    src/osmscout/util/Number.cpp
    src/osmscout/util/NumberSet.cpp
    src/osmscout/util/Parsing.cpp
    src/osmscout/util/Progress.cpp
    src/osmscout/util/Projection.cpp
    src/osmscout/util/StopClock.cpp
    src/osmscout/util/String.cpp
    src/osmscout/util/StringMatcher.cpp
    src/osmscout/util/Tiling.cpp
    src/osmscout/util/TileId.cpp
    src/osmscout/util/Transformation.cpp
    src/osmscout/util/WorkQueue.cpp
    src/osmscout/util/TagErrorReporter.cpp
    src/osmscout/routing/Route.cpp
    src/osmscout/routing/RouteData.cpp
    src/osmscout/routing/RouteNode.cpp
    src/osmscout/routing/RouteNodeDataFile.cpp
    src/osmscout/routing/RoutePostprocessor.cpp
    src/osmscout/routing/RoutingDB.cpp
    src/osmscout/routing/RoutingProfile.cpp
    src/osmscout/routing/RoutingService.cpp
    src/osmscout/routing/AbstractRoutingService.cpp
    src/osmscout/routing/SimpleRoutingService.cpp
    src/osmscout/routing/MultiDBRoutingService.cpp
    src/osmscout/routing/TurnRestriction.cpp
    src/osmscout/routing/MultiDBRoutingState.cpp
    src/osmscout/navigation/Agents.cpp
    src/osmscout/navigation/DataAgent.cpp
    src/osmscout/navigation/PositionAgent.cpp
    src/osmscout/navigation/RouteStateAgent.cpp
    src/osmscout/navigation/Engine.cpp
    src/osmscout/Area.cpp
    src/osmscout/AreaDataFile.cpp
    src/osmscout/AreaAreaIndex.cpp
    src/osmscout/AreaNodeIndex.cpp
    src/osmscout/AreaWayIndex.cpp
    src/osmscout/Coord.cpp
    src/osmscout/CoordDataFile.cpp
    src/osmscout/CoverageIndex.cpp
    src/osmscout/BoundingBoxDataFile.cpp
    src/osmscout/TypeDistributionDataFile.cpp
    src/osmscout/Database.cpp
    src/osmscout/DebugDatabase.cpp
    src/osmscout/BasemapDatabase.cpp
    src/osmscout/GeoCoord.cpp
    src/osmscout/GroundTile.cpp
    src/osmscout/Intersection.cpp
    src/osmscout/Location.cpp
    src/osmscout/LocationIndex.cpp
    src/osmscout/LocationService.cpp
    src/osmscout/LocationDescriptionService.cpp
    src/osmscout/Node.cpp
    src/osmscout/NodeDataFile.cpp
    src/osmscout/NumericIndex.cpp
    src/osmscout/ObjectRef.cpp
    src/osmscout/OptimizeAreasLowZoom.cpp
    src/osmscout/OptimizeWaysLowZoom.cpp
    src/osmscout/Path.cpp
    src/osmscout/Pixel.cpp
    src/osmscout/Point.cpp
    src/osmscout/POIService.cpp
    src/osmscout/ObjectVariantDataFile.cpp
    src/osmscout/SRTM.cpp
    src/osmscout/Tag.cpp
    src/osmscout/TypeConfig.cpp
    src/osmscout/TypeFeature.cpp
    src/osmscout/TypeFeatures.cpp
    src/osmscout/TypeInfoSet.cpp
    src/osmscout/FeatureReader.cpp
    src/osmscout/OSMScoutTypes.cpp
    src/osmscout/WaterIndex.cpp
    src/osmscout/Way.cpp
    src/osmscout/WayDataFile.cpp
    src/osmscout/util/CmdLineParsing.cpp)

if(MARISA_FOUND)
    list(APPEND HEADER_FILES include/osmscout/TextSearchIndex.h)
    list(APPEND SOURCE_FILES src/osmscout/TextSearchIndex.cpp)
endif()

if(IOS)
  add_library(OSMScout STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(OSMScout ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_target_properties(OSMScout PROPERTIES
    CXX_STANDARD 11
	OUTPUT_NAME "osmscout")

target_include_directories(OSMScout PUBLIC include ${CMAKE_SOURCE_DIR}/include)

# Public features Header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/CoreFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/CoreFeatures.h)
target_include_directories(OSMScout PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)

# Private config header
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScout")
target_include_directories(OSMScout PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/privateinclude)

if(MARISA_FOUND)
    target_include_directories(OSMScout PUBLIC ${MARISA_INCLUDE_DIRS})
    target_link_libraries(OSMScout ${MARISA_LIBRARIES})
endif()

if (ICONV_FOUND)
    target_include_directories(OSMScout PRIVATE ${ICONV_INCLUDE_DIRS})
    target_link_libraries(OSMScout ${ICONV_LIBRARIES})
endif()

if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(OSMScout ${CMAKE_THREAD_LIBS_INIT})
endif()

if(${IOS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
endif()

if(APPLE AND BUILD_FRAMEWORKS)
  set_target_properties(OSMScout PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION C
  MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  PUBLIC_HEADER     "${HEADER_FILES}"
  CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
  OUTPUT_NAME "OSMScout")

  set_property(SOURCE "${HEADER_FILES_OST}"
          PROPERTY MACOSX_PACKAGE_LOCATION Headers/ost)
  set_property(SOURCE "${HEADER_FILES_SYSTEM}"
          PROPERTY MACOSX_PACKAGE_LOCATION Headers/system)
  set_property(SOURCE "${HEADER_FILES_UTIL}"
          PROPERTY MACOSX_PACKAGE_LOCATION Headers/util)
  set_property(SOURCE "${HEADER_FILES_ROUTING}"
          PROPERTY MACOSX_PACKAGE_LOCATION Headers/routing)

endif()

target_compile_definitions(OSMScout PRIVATE -DOSMSCOUT_EXPORT_SYMBOLS)
install(TARGETS OSMScout
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)

if(MARISA_FOUND)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
else()
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE PATTERN TextSearchIndex.h EXCLUDE)
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/CoreFeatures.h DESTINATION include/osmscout)
