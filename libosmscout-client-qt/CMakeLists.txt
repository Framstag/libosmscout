if(NOT Qt5Core_FOUND)
  message(SEND_ERROR "Qt5Core is required for OSMScout Qt client library")
endif()
if(NOT Qt5Gui_FOUND)
  message(SEND_ERROR "Qt5Gui is required for OSMScout Qt client library")
endif()
if(NOT Qt5Quick_FOUND)
  message(SEND_ERROR "Qt5Quick is required for OSMScout Qt client library")
endif()
if(NOT ${OSMSCOUT_BUILD_MAP_QT})
  message(SEND_ERROR "The main map drawing interface is required for Qt map drawing backend")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(HEADER_FILES
    include/osmscout/DBThread.h
    include/osmscout/DBInstance.h
    include/osmscout/FileDownloader.h
    include/osmscout/InputHandler.h
    include/osmscout/LocationEntry.h
    include/osmscout/LocationInfoModel.h
    include/osmscout/MapWidget.h
    include/osmscout/OnlineTileProvider.h
    include/osmscout/OnlineTileProviderModel.h
    include/osmscout/OsmTileDownloader.h
    include/osmscout/OSMTile.h
    include/osmscout/private
    include/osmscout/private/ClientQtImportExport.h
    include/osmscout/RoutingModel.h
    include/osmscout/Router.h
    include/osmscout/SearchLocationModel.h
    include/osmscout/Settings.h
    include/osmscout/TileCache.h
    include/osmscout/MapProvider.h
    include/osmscout/AvailableMapsModel.h
    include/osmscout/PersistentCookieJar.h
    include/osmscout/MapManager.h
    include/osmscout/MapDownloadsModel.h
    include/osmscout/MapObjectInfoModel.h
    include/osmscout/MapStyleModel.h
    include/osmscout/StyleFlagsModel.h
    include/osmscout/OSMScoutQt.h
    include/osmscout/DBJob.h
    include/osmscout/LookupModule.h
    include/osmscout/SearchModule.h
    include/osmscout/StyleModule.h
    include/osmscout/MapRenderer.h
    include/osmscout/PlaneMapRenderer.h
    include/osmscout/TiledMapRenderer.h
    include/osmscout/OverlayObject.h
)

set(SOURCE_FILES
    src/osmscout/DBThread.cpp
    src/osmscout/DBInstance.cpp
    src/osmscout/FileDownloader.cpp
    src/osmscout/InputHandler.cpp
    src/osmscout/LocationEntry.cpp
    src/osmscout/LocationInfoModel.cpp
    src/osmscout/MapWidget.cpp
    src/osmscout/OnlineTileProvider.cpp
    src/osmscout/OnlineTileProviderModel.cpp
    src/osmscout/OSMTile.cpp
    src/osmscout/OsmTileDownloader.cpp
    src/osmscout/RoutingModel.cpp
    src/osmscout/Router.cpp
    src/osmscout/SearchLocationModel.cpp
    src/osmscout/Settings.cpp
    src/osmscout/TileCache.cpp
    src/osmscout/MapProvider.cpp
    src/osmscout/AvailableMapsModel.cpp
    src/osmscout/MapManager.cpp
    src/osmscout/MapDownloadsModel.cpp
    src/osmscout/MapObjectInfoModel.cpp
    src/osmscout/MapStyleModel.cpp
    src/osmscout/StyleFlagsModel.cpp
    src/osmscout/OSMScoutQt.cpp
    src/osmscout/DBJob.cpp
    src/osmscout/LookupModule.cpp
    src/osmscout/SearchModule.cpp
    src/osmscout/StyleModule.cpp
    src/osmscout/MapRenderer.cpp
    src/osmscout/PlaneMapRenderer.cpp
    src/osmscout/TiledMapRenderer.cpp
    src/osmscout/OverlayObject.cpp
)

if(APPLE)
  set(THE_TARGET_NAME "OSMScoutClientQt") 
else()
  set(THE_TARGET_NAME "osmscout_client_qt") 
endif()

if(IOS)
  add_library(${THE_TARGET_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(${THE_TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_property(TARGET ${THE_TARGET_NAME} PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/ClientQtFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/ClientQtFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "${THE_TARGET_NAME}")
target_include_directories(${THE_TARGET_NAME} PRIVATE 
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
    ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
    ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include
    ${Qt5Gui_INCLUDE_DIRS})
if(MARISA_FOUND)
    target_include_directories(${THE_TARGET_NAME} PRIVATE ${MARISA_INCLUDE_DIRS})
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

if(APPLE)
target_link_libraries(${THE_TARGET_NAME}
    OSMScout
    OSMScoutMap
    OSMScoutMapQt
    Qt5::Core
    Qt5::Gui
    Qt5::Quick)
else()
target_link_libraries(${THE_TARGET_NAME}
    osmscout
    osmscout_map
    osmscout_map_qt
    Qt5::Core
    Qt5::Gui
    Qt5::Quick)
endif()

target_compile_definitions(${THE_TARGET_NAME} PRIVATE -DOSMSCOUT_CLIENT_QT_EXPORT_SYMBOLS ${Qt5Gui_DEFINITIONS})
install(TARGETS ${THE_TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/ClientQtFeatures.h DESTINATION include/osmscout)
