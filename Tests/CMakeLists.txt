#---- CachePerformance
add_executable(CachePerformance src/CachePerformance.cpp)
set_property(TARGET CachePerformance PROPERTY CXX_STANDARD 11)
target_include_directories(CachePerformance PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CachePerformance osmscout)
install(TARGETS CachePerformance RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- CalculateResolution
add_executable(CalculateResolution src/CalculateResolution.cpp)
set_property(TARGET CalculateResolution PROPERTY CXX_STANDARD 11)
target_include_directories(CalculateResolution PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CalculateResolution osmscout)
install(TARGETS CalculateResolution RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- CoordinateCalculations
add_executable(CoordinateCalculations src/CoordinateCalculations.cpp)
set_property(TARGET CoordinateCalculations PROPERTY CXX_STANDARD 11)
target_include_directories(CoordinateCalculations PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CoordinateCalculations osmscout)
install(TARGETS CoordinateCalculations RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- CoordinateEncoding
add_executable(CoordinateEncoding src/CoordinateEncoding.cpp)
set_property(TARGET CoordinateEncoding PROPERTY CXX_STANDARD 11)
target_include_directories(CoordinateEncoding PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CoordinateEncoding osmscout)
install(TARGETS CoordinateEncoding RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- NumberSetPerformance
add_executable(NumberSetPerformance src/NumberSetPerformance.cpp)
set_property(TARGET NumberSetPerformance PROPERTY CXX_STANDARD 11)
target_include_directories(NumberSetPerformance PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(NumberSetPerformance osmscout)
install(TARGETS NumberSetPerformance RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- ReaderScannerPerformance
add_executable(ReaderScannerPerformance src/ReaderScannerPerformance.cpp)
set_property(TARGET ReaderScannerPerformance PROPERTY CXX_STANDARD 11)
target_include_directories(ReaderScannerPerformance PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(ReaderScannerPerformance osmscout)
install(TARGETS ReaderScannerPerformance RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- ThreadedDatabase
if(${OSMSCOUT_BUILD_MAP})
	add_executable(ThreadedDatabase src/ThreadedDatabase.cpp)
	set_property(TARGET ThreadedDatabase PROPERTY CXX_STANDARD 11)
	target_include_directories(ThreadedDatabase PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)
	target_link_libraries(ThreadedDatabase osmscout osmscout_map)
	install(TARGETS ThreadedDatabase RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
else()
	message("Skip ThreadedDatabase test libosmscout-map, is missing.")
endif()


if(${OSMSCOUT_BUILD_MAP_QT})
    set(CMAKE_AUTOMOC ON)

    add_executable(DrawTextQt src/DrawTextQt.cpp include/DrawWindow.h)
    set_property(TARGET DrawTextQt PROPERTY CXX_STANDARD 11)
    target_include_directories(DrawTextQt PRIVATE include ${CMAKE_CURRENT_BINARY_DIR} ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})
    target_link_libraries(DrawTextQt osmscout osmscout_map osmscout_map_qt ${Qt5Widgets_LIBRARIES})
    install(TARGETS DrawTextQt RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
    target_compile_definitions(DrawTextQt PRIVATE ${Qt5Widgets_DEFINITIONS})
else()
	message("Skip DrawTextQt test, libosmscout-map-qt is missing.")
endif()

#---- WorkQueue
add_executable(WorkQueue src/WorkQueue.cpp)
set_property(TARGET WorkQueue PROPERTY CXX_STANDARD 11)
target_include_directories(WorkQueue PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(WorkQueue osmscout)
install(TARGETS WorkQueue RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- MapRotate
if(${OSMSCOUT_BUILD_MAP})
  add_executable(MapRotate src/MapRotate.cpp)
  set_property(TARGET MapRotate PROPERTY CXX_STANDARD 11)
  target_include_directories(MapRotate PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)
  target_link_libraries(MapRotate osmscout osmscout_map)
  install(TARGETS MapRotate RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
else()
	message("Skip MapRotate test libosmscout-map, is missing.")
endif()
