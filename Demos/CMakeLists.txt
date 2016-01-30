#---- DumpOSS

add_executable(DumpOSS src/DumpOSS.cpp)
set_property(TARGET DumpOSS PROPERTY CXX_STANDARD 11)
target_include_directories(DumpOSS PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)
target_link_libraries(DumpOSS libosmscout libosmscout_map)
install(TARGETS DumpOSS RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- LocationDescription

add_executable(LocationDescription src/LocationDescription.cpp)
set_property(TARGET LocationDescription PROPERTY CXX_STANDARD 11)
target_include_directories(LocationDescription PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(LocationDescription libosmscout)
install(TARGETS LocationDescription RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- LocationLookup

add_executable(LocationLookup src/LocationLookup.cpp)
set_property(TARGET LocationLookup PROPERTY CXX_STANDARD 11)
target_include_directories(LocationLookup PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(LocationLookup libosmscout)
install(TARGETS LocationLookup RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- LookupPOI

add_executable(LookupPOI src/LookupPOI.cpp)
set_property(TARGET LookupPOI PROPERTY CXX_STANDARD 11)
target_include_directories(LookupPOI PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(LookupPOI libosmscout)
install(TARGETS LookupPOI RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- LookupText

add_executable(LookupText src/LookupText.cpp)
set_property(TARGET LookupText PROPERTY CXX_STANDARD 11)
target_include_directories(LookupText PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${MARISA_INCLUDE_DIRS})
target_link_libraries(LookupText libosmscout)
install(TARGETS LookupText RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- PerformanceTest

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/PerformanceTestConfig.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/PerformanceTest/config.h)
add_executable(PerformanceTest src/PerformanceTest.cpp)
set_property(TARGET PerformanceTest PROPERTY CXX_STANDARD 11)
target_include_directories(PerformanceTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/PerformanceTest ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)
target_link_libraries(PerformanceTest libosmscout libosmscout_map)
if(CAIRO_FOUND)
	target_include_directories(PerformanceTest PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-cairo/include ${CAIRO_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
	target_link_libraries(PerformanceTest libosmscout_map_cairo)
endif()
if(Qt5Gui_FOUND)
	target_include_directories(PerformanceTest PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include ${Qt5Gui_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})
	target_link_libraries(PerformanceTest libosmscout_map_qt ${Qt5Widgets_LIBRARIES})
endif()
install(TARGETS PerformanceTest RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- ResourceConsumption

add_executable(ResourceConsumption src/ResourceConsumption.cpp)
set_property(TARGET ResourceConsumption PROPERTY CXX_STANDARD 11)
target_include_directories(ResourceConsumption PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)
target_link_libraries(ResourceConsumption libosmscout libosmscout_map)
install(TARGETS ResourceConsumption RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- ReverseLocationLookup

add_executable(ReverseLocationLookup src/ReverseLocationLookup.cpp)
set_property(TARGET ReverseLocationLookup PROPERTY CXX_STANDARD 11)
target_include_directories(ReverseLocationLookup PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(ReverseLocationLookup libosmscout)
install(TARGETS ReverseLocationLookup RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- Routing

add_executable(Routing src/Routing.cpp)
set_property(TARGET Routing PROPERTY CXX_STANDARD 11)
target_include_directories(Routing PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(Routing libosmscout)
install(TARGETS Routing RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#----

if(LIBAGG_FOUND)
    add_executable(Tiler src/Tiler.cpp)
	set_property(TARGET Tiler PROPERTY CXX_STANDARD 11)
	target_include_directories(Tiler PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-agg/include ${LIBAGG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
	target_link_libraries(Tiler libosmscout libosmscout_map libosmscout_map_agg)
	install(TARGETS Tiler RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

    add_executable(DrawMapAgg src/DrawMapAgg.cpp)
	set_property(TARGET DrawMapAgg PROPERTY CXX_STANDARD 11)
	target_include_directories(DrawMapAgg PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-agg/include ${LIBAGG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
	target_link_libraries(DrawMapAgg libosmscout libosmscout_map libosmscout_map_agg)
	install(TARGETS DrawMapAgg RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
endif()

if(CAIRO_FOUND)
    add_executable(DrawMapCairo src/DrawMapCairo.cpp)
	set_property(TARGET DrawMapCairo PROPERTY CXX_STANDARD 11)
	target_include_directories(DrawMapCairo PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-cairo/include ${CAIRO_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
	target_link_libraries(DrawMapCairo libosmscout libosmscout_map libosmscout_map_cairo)
	install(TARGETS DrawMapCairo RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
endif()

if(Qt5Gui_FOUND)
    add_executable(DrawMapQt src/DrawMapQt.cpp)
	set_property(TARGET DrawMapQt PROPERTY CXX_STANDARD 11)
	target_include_directories(DrawMapQt PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})
	target_link_libraries(DrawMapQt libosmscout libosmscout_map libosmscout_map_qt ${Qt5Widgets_LIBRARIES})
	install(TARGETS DrawMapQt RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
	target_compile_definitions(DrawMapQt PRIVATE ${Qt5Widgets_DEFINITIONS})

    add_executable(ResourceConsumptionQt src/ResourceConsumptionQt.cpp)
	set_property(TARGET ResourceConsumptionQt PROPERTY CXX_STANDARD 11)
	target_include_directories(ResourceConsumptionQt PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})
	target_link_libraries(ResourceConsumptionQt libosmscout libosmscout_map libosmscout_map_qt ${Qt5Widgets_LIBRARIES})
	install(TARGETS ResourceConsumptionQt RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
	target_compile_definitions(ResourceConsumptionQt PRIVATE ${Qt5Widgets_DEFINITIONS})
endif()

add_executable(DrawMapSVG src/DrawMapSVG.cpp)
set_property(TARGET DrawMapSVG PROPERTY CXX_STANDARD 11)
target_include_directories(DrawMapSVG PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-svg/include)
target_link_libraries(DrawMapSVG libosmscout libosmscout_map libosmscout_map_svg)
install(TARGETS DrawMapSVG RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
