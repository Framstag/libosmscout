# Uncomment to force marisa lib
#set(MARISA_FOUND 1)

set(HEADER_FILES_ROOT
        include/osmscout/Area.h
        include/osmscout/GeoCoord.h
        include/osmscout/GroundTile.h
        include/osmscout/Intersection.h
        include/osmscout/Node.h
        include/osmscout/ObjectRef.h
        include/osmscout/Path.h
        include/osmscout/Pixel.h
        include/osmscout/Point.h
        include/osmscout/PublicTransport.h
        include/osmscout/Route.h
        include/osmscout/Tag.h
        include/osmscout/TypeConfig.h
        include/osmscout/TypeFeature.h
        include/osmscout/TypeInfoSet.h
        include/osmscout/FeatureReader.h
        include/osmscout/OSMScoutTypes.h
        include/osmscout/Way.h)

set(HEADER_FILES_LIB
        include/osmscout/lib/CoreFeatures.h
        include/osmscout/lib/CoreImportExport.h)

set(HEADER_FILES_LOG
        include/osmscout/log/Logger.h
        include/osmscout/log/LoggerImpl.h)

set(HEADER_FILES_CLI
        include/osmscout/cli/CmdLineParsing.h)

set(HEADER_FILES_ASYNC
        include/osmscout/async/AsyncWorker.h
        include/osmscout/async/Breaker.h
        include/osmscout/async/CancelableFuture.h
        include/osmscout/async/ProcessingQueue.h
        include/osmscout/async/Signal.h
        include/osmscout/async/Thread.h
        include/osmscout/async/Worker.h
        include/osmscout/async/WorkQueue.h)

set(HEADER_FILES_FEATURE
        include/osmscout/feature/AccessFeature.h
        include/osmscout/feature/AccessRestrictedFeature.h
        include/osmscout/feature/AddressFeature.h
        include/osmscout/feature/AdminLevelFeature.h
        include/osmscout/feature/BridgeFeature.h
        include/osmscout/feature/BuildingFeature.h
        include/osmscout/feature/ClockwiseDirectionFeature.h
        include/osmscout/feature/ColorFeature.h
        include/osmscout/feature/ConstructionYearFeature.h
        include/osmscout/feature/DestinationFeature.h
        include/osmscout/feature/EleFeature.h
        include/osmscout/feature/EmbankmentFeature.h
        include/osmscout/feature/FromToFeature.h
        include/osmscout/feature/GradeFeature.h
        include/osmscout/feature/IsInFeature.h
        include/osmscout/feature/LanesFeature.h
        include/osmscout/feature/LayerFeature.h
        include/osmscout/feature/LocationFeature.h
        include/osmscout/feature/MaxSpeedFeature.h
        include/osmscout/feature/NameFeature.h
        include/osmscout/feature/NameAltFeature.h
        include/osmscout/feature/NameShortFeature.h
        include/osmscout/feature/NetworkFeature.h
        include/osmscout/feature/OpeningHoursFeature.h
        include/osmscout/feature/OperatorFeature.h
        include/osmscout/feature/PhoneFeature.h
        include/osmscout/feature/PostalCodeFeature.h
        include/osmscout/feature/RefFeature.h
        include/osmscout/feature/RoundaboutFeature.h
        include/osmscout/feature/SidewayFeature.h
        include/osmscout/feature/TunnelFeature.h
        include/osmscout/feature/WebsiteFeature.h
        include/osmscout/feature/WidthFeature.h)

set(HEADER_FILES_IO
        include/osmscout/io/DataFile.h
        include/osmscout/io/File.h
        include/osmscout/io/FileScanner.h
        include/osmscout/io/FileWriter.h
        include/osmscout/io/NumericIndex.h)

set(HEADER_FILES_DB
        include/osmscout/db/AreaAreaIndex.h
        include/osmscout/db/AreaDataFile.h
        include/osmscout/db/AreaIndex.h
        include/osmscout/db/AreaNodeIndex.h
        include/osmscout/db/AreaRouteIndex.h
        include/osmscout/db/AreaWayIndex.h
        include/osmscout/db/CoordDataFile.h
        include/osmscout/db/CoverageIndex.h
        include/osmscout/db/BoundingBoxDataFile.h
        include/osmscout/db/TypeDistributionDataFile.h
        include/osmscout/db/Database.h
        include/osmscout/db/BasemapDatabase.h
        include/osmscout/db/DebugDatabase.h
        include/osmscout/db/LocationIndex.h
        include/osmscout/db/NodeDataFile.h
        include/osmscout/db/OptimizeAreasLowZoom.h
        include/osmscout/db/OptimizeWaysLowZoom.h
        include/osmscout/db/PTRouteDataFile.h
        include/osmscout/db/ObjectVariantDataFile.h
        include/osmscout/db/WaterIndex.h
        include/osmscout/db/WayDataFile.h)

set(HEADER_FILES_LOCATION
        include/osmscout/location/Location.h
        include/osmscout/location/LocationService.h
        include/osmscout/location/LocationDescriptionService.h)


set(HEADER_FILES_NAVIGATION
        include/osmscout/navigation/Agents.h
        include/osmscout/navigation/ArrivalEstimateAgent.h
        include/osmscout/navigation/DataAgent.h
        include/osmscout/navigation/PositionAgent.h
        include/osmscout/navigation/RouteStateAgent.h
        include/osmscout/navigation/Engine.h
        include/osmscout/navigation/Navigation.h
        include/osmscout/navigation/BearingAgent.h
        include/osmscout/navigation/RouteInstructionAgent.h
        include/osmscout/navigation/SpeedAgent.h
        include/osmscout/navigation/VoiceInstructionAgent.h
        include/osmscout/navigation/LaneAgent.h)

set(HEADER_FILES_OST
        include/osmscout/ost/Parser.h
        include/osmscout/ost/Scanner.h)

set(HEADER_FILES_PROJECTION
        include/osmscout/projection/Earth.h
        include/osmscout/projection/MercatorProjection.h
        include/osmscout/projection/Projection.h
        include/osmscout/projection/TileProjection.h)

set(HEADER_FILES_POI
        include/osmscout/poi/POIService.h)

set(HEADER_FILES_ELEVATION
        include/osmscout/elevation/ElevationService.h
        include/osmscout/elevation/SRTM.h)

set(HEADER_FILES_ROUTING
        include/osmscout/routing/RouteData.h
        include/osmscout/routing/RouteDataFile.h
        include/osmscout/routing/RouteDescription.h
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
        include/osmscout/routing/MultiDBRoutingState.h
        include/osmscout/routing/RouteDescriptionPostprocessor.h)

set(HEADER_FILES_SYSTEM
    include/osmscout/system/Assert.h
    include/osmscout/system/Compiler.h
    include/osmscout/system/Math.h
    include/osmscout/system/SSEMath.h
    include/osmscout/system/SSEMathPublic.h
    include/osmscout/system/SystemTypes.h)

set(HEADER_FILES_UTIL
    include/osmscout/util/Base64.h
    include/osmscout/util/Bearing.h
    include/osmscout/util/Cache.h
    include/osmscout/util/Color.h
    include/osmscout/util/Distance.h
    include/osmscout/util/Exception.h
    include/osmscout/util/HTMLWriter.h
    include/osmscout/util/LaneTurn.h
    include/osmscout/util/Locale.h
    include/osmscout/util/GeoBox.h
    include/osmscout/util/Geometry.h
    include/osmscout/util/Magnification.h
    include/osmscout/util/MemoryMonitor.h
    include/osmscout/util/NodeUseMap.h
    include/osmscout/util/Number.h
    include/osmscout/util/NumberSet.h
    include/osmscout/util/ObjectPool.h
    include/osmscout/util/OpeningHours.h
    include/osmscout/util/Parsing.h
    include/osmscout/util/PolygonCenter.h
    include/osmscout/util/Progress.h
    include/osmscout/util/ScreenBox.h
    include/osmscout/util/ScopeGuard.h
    include/osmscout/util/StopClock.h
    include/osmscout/util/String.h
    include/osmscout/util/StringMatcher.h
    include/osmscout/util/SunriseSunset.h
    include/osmscout/util/TagErrorReporter.h
    include/osmscout/util/Tiling.h
    include/osmscout/util/Time.h
    include/osmscout/util/TileId.h
    include/osmscout/util/Transformation.h
    include/osmscout/util/utf8helper.h
    include/osmscout/util/utf8helper_charmap.h)

set(SOURCE_FILES
    src/osmscout/log/Logger.cpp
    src/osmscout/log/LoggerImpl.cpp
    src/osmscout/io/File.cpp
    src/osmscout/io/FileScanner.cpp
    src/osmscout/io/FileWriter.cpp
    src/osmscout/io/NumericIndex.cpp
    src/osmscout/async/AsyncWorker.cpp
    src/osmscout/async/Breaker.cpp
    src/osmscout/async/Thread.cpp
    src/osmscout/async/Worker.cpp
    src/osmscout/async/WorkQueue.cpp
    src/osmscout/feature/AccessFeature.cpp
    src/osmscout/feature/AccessRestrictedFeature.cpp
    src/osmscout/feature/AddressFeature.cpp
    src/osmscout/feature/AdminLevelFeature.cpp
    src/osmscout/feature/BridgeFeature.cpp
    src/osmscout/feature/BuildingFeature.cpp
    src/osmscout/feature/ClockwiseDirectionFeature.cpp
    src/osmscout/feature/ColorFeature.cpp
    src/osmscout/feature/ConstructionYearFeature.cpp
    src/osmscout/feature/DestinationFeature.cpp
    src/osmscout/feature/EleFeature.cpp
    src/osmscout/feature/EmbankmentFeature.cpp
    src/osmscout/feature/FromToFeature.cpp
    src/osmscout/feature/GradeFeature.cpp
    src/osmscout/feature/IsInFeature.cpp
    src/osmscout/feature/LanesFeature.cpp
    src/osmscout/feature/LayerFeature.cpp
    src/osmscout/feature/LocationFeature.cpp
    src/osmscout/feature/MaxSpeedFeature.cpp
    src/osmscout/feature/NameFeature.cpp
    src/osmscout/feature/NameAltFeature.cpp
    src/osmscout/feature/NameShortFeature.cpp
    src/osmscout/feature/NetworkFeature.cpp
    src/osmscout/feature/OpeningHoursFeature.cpp
    src/osmscout/feature/OperatorFeature.cpp
    src/osmscout/feature/PhoneFeature.cpp
    src/osmscout/feature/PostalCodeFeature.cpp
    src/osmscout/feature/RefFeature.cpp
    src/osmscout/feature/RoundaboutFeature.cpp
    src/osmscout/feature/SidewayFeature.cpp
    src/osmscout/feature/TunnelFeature.cpp
    src/osmscout/feature/WebsiteFeature.cpp
    src/osmscout/feature/WidthFeature.cpp
    src/osmscout/navigation/Agents.cpp
    src/osmscout/navigation/ArrivalEstimateAgent.cpp
    src/osmscout/navigation/BearingAgent.cpp
    src/osmscout/navigation/DataAgent.cpp
    src/osmscout/navigation/PositionAgent.cpp
    src/osmscout/navigation/RouteStateAgent.cpp
    src/osmscout/navigation/RouteInstructionAgent.cpp
    src/osmscout/navigation/SpeedAgent.cpp
    src/osmscout/navigation/Engine.cpp
    src/osmscout/navigation/VoiceInstructionAgent.cpp
    src/osmscout/navigation/LaneAgent.cpp
    src/osmscout/ost/Parser.cpp
    src/osmscout/ost/Scanner.cpp
    src/osmscout/projection/Earth.cpp
    src/osmscout/projection/MercatorProjection.cpp
    src/osmscout/projection/Projection.cpp
    src/osmscout/projection/TileProjection.cpp
    src/osmscout/routing/RouteData.cpp
    src/osmscout/routing/RouteDescription.cpp
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
    src/osmscout/routing/RouteDescriptionPostprocessor.cpp
    src/osmscout/routing/RouteDataFile.cpp
    src/osmscout/system/SSEMath.cpp
    src/osmscout/util/Bearing.cpp
    src/osmscout/util/Cache.cpp
    src/osmscout/util/Color.cpp
    src/osmscout/util/Distance.cpp
    src/osmscout/util/Exception.cpp
    src/osmscout/util/HTMLWriter.cpp
    src/osmscout/util/LaneTurn.cpp
    src/osmscout/util/Locale.cpp
    src/osmscout/util/GeoBox.cpp
    src/osmscout/util/Geometry.cpp
    src/osmscout/util/Magnification.cpp
    src/osmscout/util/MemoryMonitor.cpp
    src/osmscout/util/NodeUseMap.cpp
    src/osmscout/util/Number.cpp
    src/osmscout/util/NumberSet.cpp
    src/osmscout/util/OpeningHours.cpp
    src/osmscout/util/Parsing.cpp
    src/osmscout/util/PolygonCenter.cpp
    src/osmscout/util/Progress.cpp
    src/osmscout/util/ScreenBox.cpp
    src/osmscout/util/StopClock.cpp
    src/osmscout/util/String.cpp
    src/osmscout/util/StringMatcher.cpp
    src/osmscout/util/Tiling.cpp
    src/osmscout/util/TileId.cpp
    src/osmscout/util/Transformation.cpp
    src/osmscout/util/utf8helper.cpp
    src/osmscout/util/utf8helper_charmap.cpp
    src/osmscout/util/SunriseSunset.cpp
    src/osmscout/util/TagErrorReporter.cpp
    src/osmscout/db/AreaDataFile.cpp
    src/osmscout/db/AreaAreaIndex.cpp
    src/osmscout/db/AreaIndex.cpp
    src/osmscout/db/AreaNodeIndex.cpp
    src/osmscout/db/AreaRouteIndex.cpp
    src/osmscout/db/AreaWayIndex.cpp
    src/osmscout/db/CoordDataFile.cpp
    src/osmscout/db/CoverageIndex.cpp
    src/osmscout/db/BoundingBoxDataFile.cpp
    src/osmscout/db/TypeDistributionDataFile.cpp
    src/osmscout/db/Database.cpp
    src/osmscout/db/DebugDatabase.cpp
    src/osmscout/db/BasemapDatabase.cpp
    src/osmscout/db/LocationIndex.cpp
    src/osmscout/db/NodeDataFile.cpp
    src/osmscout/db/OptimizeAreasLowZoom.cpp
    src/osmscout/db/OptimizeWaysLowZoom.cpp
    src/osmscout/db/PTRouteDataFile.cpp
    src/osmscout/db/ObjectVariantDataFile.cpp
    src/osmscout/db/WaterIndex.cpp
    src/osmscout/db/WayDataFile.cpp
    src/osmscout/location/Location.cpp
    src/osmscout/location/LocationService.cpp
    src/osmscout/location/LocationDescriptionService.cpp
    src/osmscout/poi/POIService.cpp
    src/osmscout/elevation/SRTM.cpp
    src/osmscout/Area.cpp
    src/osmscout/GeoCoord.cpp
    src/osmscout/GroundTile.cpp
    src/osmscout/Intersection.cpp
    src/osmscout/Node.cpp
    src/osmscout/ObjectRef.cpp
    src/osmscout/Path.cpp
    src/osmscout/Pixel.cpp
    src/osmscout/Point.cpp
    src/osmscout/PublicTransport.cpp
    src/osmscout/Route.cpp
    src/osmscout/Tag.cpp
    src/osmscout/TypeConfig.cpp
    src/osmscout/TypeFeature.cpp
    src/osmscout/TypeInfoSet.cpp
    src/osmscout/FeatureReader.cpp
    src/osmscout/OSMScoutTypes.cpp
    src/osmscout/Way.cpp
    src/osmscout/cli/CmdLineParsing.cpp)

set(EXCLUDE_HEADER)

if(MARISA_FOUND)
    list(APPEND HEADER_FILES_ROOT include/osmscout/db/TextSearchIndex.h)
    list(APPEND SOURCE_FILES src/osmscout/db/TextSearchIndex.cpp)
else()
    list(APPEND EXCLUDE_HEADER TextSearchIndex.h)
endif()

set(HEADER_FILES
    ${HEADER_FILES_ROOT}
    ${HEADER_FILES_LIB}
    ${HEADER_FILES_LOG}
    ${HEADER_FILES_CLI}
    ${HEADER_FILES_ASYNC}
    ${HEADER_FILES_FEATURE}
    ${HEADER_FILES_IO}
    ${HEADER_FILES_DB}
    ${HEADER_FILES_ELEVATION}
    ${HEADER_FILES_LOCATION}
    ${HEADER_FILES_POI}
    ${HEADER_FILES_NAVIGATION}
    ${HEADER_FILES_OST}
    ${HEADER_FILES_PROJECTION}
    ${HEADER_FILES_ROUTING}
    ${HEADER_FILES_SYSTEM}
    ${HEADER_FILES_UTIL})

osmscout_library_project(
    NAME OSMScout
    ALIAS OSMScout
    OUTPUT_NAME "osmscout"
    SOURCE ${SOURCE_FILES}
    HEADER ${HEADER_FILES}
    TEMPLATE  ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/lib/CoreFeatures.h.cmake
    EXCLUDE ${EXCLUDE_HEADER}
)

if(MARISA_FOUND)
    target_include_directories(OSMScout PRIVATE ${MARISA_INCLUDE_DIRS})
    target_link_libraries(OSMScout ${MARISA_LIBRARIES})
endif()

if (TARGET Iconv::Iconv)
    target_link_libraries(OSMScout Iconv::Iconv)
endif()

if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(OSMScout ${CMAKE_THREAD_LIBS_INIT})
endif()

if(CMAKE_COMPILER_IS_GNUCXX AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS_EQUAL "8.3"))
    # GCC <= 8.3 needs explicit filesystem library linking
    target_link_libraries(OSMScout -lstdc++fs)
endif()

if(APPLE AND OSMSCOUT_BUILD_FRAMEWORKS)
    set_source_files_properties(${HEADER_FILES_ROOT}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers)
    set_source_files_properties(${HEADER_FILES_LIB}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/lib)
    set_source_files_properties(${HEADER_FILES_LOG}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/log)
    set_source_files_properties(${HEADER_FILES_CLI}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/cli)
    set_source_files_properties(${HEADER_FILES_ASYNC}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/async)
    set_source_files_properties(${HEADER_FILES_FEATURE}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/feature)
    set_source_files_properties(${HEADER_FILES_IO}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/io)
    set_source_files_properties(${HEADER_FILES_DB}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/db)
    set_source_files_properties(${HEADER_FILES_ELEVATION}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/elevation)
    set_source_files_properties(${HEADER_FILES_LOCATION}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/location)
    set_source_files_properties(${HEADER_FILES_POI}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/poi)
    set_source_files_properties(${HEADER_FILES_OST}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/ost)
    set_source_files_properties(${HEADER_FILES_SYSTEM}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/system)
    set_source_files_properties(${HEADER_FILES_UTIL}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/util)
    set_source_files_properties(${HEADER_FILES_ROUTING}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/routing)
    set_source_files_properties(${HEADER_FILES_NAVIGATION}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/navigation)
    set_source_files_properties(${HEADER_FILES_PROJECTION}
            PROPERTIES MACOSX_PACKAGE_LOCATION Headers/projection)

    set_target_properties(OSMScout PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
        #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
        CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        OUTPUT_NAME "OSMScout")

     install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/lib/CoreFeatures.h DESTINATION include/osmscout/lib)
