cmake_minimum_required(VERSION 3.1)
project(libosmscout_project CXX)

add_subdirectory(libosmscout)

add_subdirectory(libosmscout-import)

add_subdirectory(libosmscout-map)
add_subdirectory(libosmscout-map-agg)
add_subdirectory(libosmscout-map-cairo)
add_subdirectory(libosmscout-map-opengl)
add_subdirectory(libosmscout-map-qt)
add_subdirectory(libosmscout-map-svg)

add_subdirectory(Import)
add_subdirectory(Demos)
add_subdirectory(DumpData)
add_subdirectory(OSMScout2)
add_subdirectory(Tests)
