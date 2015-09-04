cmake_minimum_required(VERSION 3.2)
project(Demos)

include(FindPkgConfig)

pkg_check_modules(LIBOSMSCOUT REQUIRED libosmscout)
pkg_check_modules(LIBOSMSCOUTMAP REQUIRED libosmscout-map)
pkg_check_modules(LIBOSMSCOUTMAPAGG libosmscout-map-agg)
pkg_check_modules(LIBOSMSCOUTMAPCAIRO libosmscout-map-cairo)
pkg_check_modules(LIBOSMSCOUTMAPQT libosmscout-map-qt Qt5Widgets>=5.0.0)
pkg_check_modules(LIBOSMSCOUTMAPSVG libosmscout-map-svg)

include_directories(${LIBOSMSCOUT_INCLUDE_DIRS}
        ${LIBOSMSCOUTMAP_INCLUDE_DIRS}
        ${LIBOSMSCOUTMAPAGG_INCLUDE_DIRS}
        ${LIBOSMSCOUTMAPCAIRO_INCLUDE_DIRS}
        ${LIBOSMSCOUTMAPQT_INCLUDE_DIRS}
        ${LIBOSMSCOUTMAPSVG_INCLUDE_DIRS})

link_directories(${LIBOSMSCOUT_LIBRARY_DIRS}
        ${LIBOSMSCOUTMAP_LIBRARY_DIRS}
        ${LIBOSMSCOUTMAPAGG_LIBRARY_DIRS}
        ${LIBOSMSCOUTMAPCAIRO_LIBRARY_DIRS}
        ${LIBOSMSCOUTMAPQT_LIBRARY_DIRS}
        ${LIBOSMSCOUTMAPSVG_LIBRARY_DIRS})

#---- DumpOSS

add_executable(DumpOSS src/DumpOSS.cpp)

target_link_libraries(DumpOSS
        ${LIBOSMSCOUT_LIBRARIES}
        ${LIBOSMSCOUTMAP_LIBRARIES})

set_property(TARGET DumpOSS PROPERTY CXX_STANDARD 11)

#---- LocationLookuo

add_executable(LocationLookup src/LocationLookup.cpp)

target_link_libraries(LocationLookup
        ${LIBOSMSCOUT_LIBRARIES})

set_property(TARGET LocationLookup PROPERTY CXX_STANDARD 11)

#---- LookupPOI

add_executable(LookupPOI src/LookupPOI.cpp)

target_link_libraries(LookupPOI
        ${LIBOSMSCOUT_LIBRARIES})

set_property(TARGET LookupPOI PROPERTY CXX_STANDARD 11)

#---- LookupText

add_executable(LookupText src/LookupText.cpp)

target_link_libraries(LookupText
        ${LIBOSMSCOUT_LIBRARIES})

set_property(TARGET LookupText PROPERTY CXX_STANDARD 11)

#---- PerformanceTest

add_executable(PerformanceTest src/PerformanceTest.cpp)

target_link_libraries(PerformanceTest
        ${LIBOSMSCOUT_LIBRARIES}
        ${LIBOSMSCOUTMAP_LIBRARIES}
        ${LIBOSMSCOUTMAPCAIRO_LIBRARIES}
        ${LIBOSMSCOUTMAPQT_LIBRARIES})

set_property(TARGET PerformanceTest PROPERTY CXX_STANDARD 11)

#---- Routing

add_executable(Routing src/Routing.cpp)

target_link_libraries(Routing
        ${LIBOSMSCOUT_LIBRARIES})

set_property(TARGET Routing PROPERTY CXX_STANDARD 11)

#----

if (${LIBOSMSCOUTMAPAGG_FOUND})
    add_executable(Tiler src/Tiler.cpp)

    target_link_libraries(Tiler
            ${LIBOSMSCOUT_LIBRARIES}
            ${LIBOSMSCOUTMAP_LIBRARIES}
            ${LIBOSMSCOUTMAPAGG_LIBRARIES})

    set_property(TARGET Tiler PROPERTY CXX_STANDARD 11)

    add_executable(DrawMapAgg src/DrawMapAgg.cpp)

    target_link_libraries(DrawMapAgg
            ${LIBOSMSCOUT_LIBRARIES}
            ${LIBOSMSCOUTMAP_LIBRARIES}
            ${LIBOSMSCOUTMAPAGG_LIBRARIES})

    set_property(TARGET DrawMapAgg PROPERTY CXX_STANDARD 11)

endif (${LIBOSMSCOUTMAPAGG_FOUND})

if (${LIBOSMSCOUTMAPCAIRO_FOUND})
    add_executable(DrawMapCairo src/DrawMapCairo.cpp)

    target_link_libraries(DrawMapCairo
            ${LIBOSMSCOUT_LIBRARIES}
            ${LIBOSMSCOUTMAP_LIBRARIES}
            ${LIBOSMSCOUTMAPCAIRO_LIBRARIES})

    set_property(TARGET DrawMapCairo PROPERTY CXX_STANDARD 11)
endif (${LIBOSMSCOUTMAPCAIRO_FOUND})

if (${LIBOSMSCOUTMAPQT_FOUND})
    add_executable(DrawMapQt src/DrawMapQt.cpp)

    target_link_libraries(DrawMapQt
            ${LIBOSMSCOUT_LIBRARIES}
            ${LIBOSMSCOUTMAP_LIBRARIES}
            ${LIBOSMSCOUTMAPQT_LIBRARIES})

    set_property(TARGET DrawMapQt PROPERTY CXX_STANDARD 11)
endif (${LIBOSMSCOUTMAPQT_FOUND})

if (${LIBOSMSCOUTMAPSVG_FOUND})
    add_executable(DrawMapSVG src/DrawMapSVG.cpp)

    target_link_libraries(DrawMapSVG
            ${LIBOSMSCOUT_LIBRARIES}
            ${LIBOSMSCOUTMAP_LIBRARIES}
            ${LIBOSMSCOUTMAPSVG_LIBRARIES})

    set_property(TARGET DrawMapSVG PROPERTY CXX_STANDARD 11)
endif (${LIBOSMSCOUTMAPSVG_FOUND})
