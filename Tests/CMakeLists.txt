#---- CachePerformance
add_executable(CachePerformance src/CachePerformance.cpp)
set_property(TARGET CachePerformance PROPERTY CXX_STANDARD 11)
target_include_directories(CachePerformance PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CachePerformance osmscout)

#---- CalculateResolution
add_executable(CalculateResolution src/CalculateResolution.cpp)
set_property(TARGET CalculateResolution PROPERTY CXX_STANDARD 11)
target_include_directories(CalculateResolution PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CalculateResolution osmscout)

#---- CoordinateCalculations
add_executable(CoordinateCalculations src/CoordinateCalculations.cpp)
set_property(TARGET CoordinateCalculations PROPERTY CXX_STANDARD 11)
target_include_directories(CoordinateCalculations PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CoordinateCalculations osmscout)

#---- CoordinateEncoding
add_executable(CoordinateEncoding src/CoordinateEncoding.cpp)
set_property(TARGET CoordinateEncoding PROPERTY CXX_STANDARD 11)
target_include_directories(CoordinateEncoding PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CoordinateEncoding osmscout)

#---- NumberSetPerformance
add_executable(NumberSetPerformance src/NumberSetPerformance.cpp)
set_property(TARGET NumberSetPerformance PROPERTY CXX_STANDARD 11)
target_include_directories(NumberSetPerformance PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(NumberSetPerformance osmscout)

#---- ReaderScannerPerformance
add_executable(ReaderScannerPerformance src/ReaderScannerPerformance.cpp)
set_property(TARGET ReaderScannerPerformance PROPERTY CXX_STANDARD 11)
target_include_directories(ReaderScannerPerformance PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(ReaderScannerPerformance osmscout)

#---- MultiDBRouting
add_executable(MultiDBRouting src/MultiDBRouting.cpp)
set_property(TARGET MultiDBRouting PROPERTY CXX_STANDARD 11)
target_include_directories(MultiDBRouting PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(MultiDBRouting osmscout)

#---- ThreadedDatabase
if(${OSMSCOUT_BUILD_MAP})
	add_executable(ThreadedDatabase src/ThreadedDatabase.cpp)
	set_property(TARGET ThreadedDatabase PROPERTY CXX_STANDARD 11)
	target_include_directories(ThreadedDatabase PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)
	target_link_libraries(ThreadedDatabase osmscout osmscout_map)
else()
	message("Skip ThreadedDatabase test libosmscout-map, is missing.")
endif()


if(${OSMSCOUT_BUILD_MAP_QT})
    set(CMAKE_AUTOMOC ON)

    add_executable(DrawTextQt src/DrawTextQt.cpp include/DrawWindow.h)
    set_property(TARGET DrawTextQt PROPERTY CXX_STANDARD 11)
    target_include_directories(DrawTextQt PRIVATE include ${CMAKE_CURRENT_BINARY_DIR} ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-qt/include ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})
    target_link_libraries(DrawTextQt osmscout osmscout_map osmscout_map_qt ${Qt5Widgets_LIBRARIES})
    target_compile_definitions(DrawTextQt PRIVATE ${Qt5Widgets_DEFINITIONS})
else()
	message("Skip DrawTextQt test, libosmscout-map-qt is missing.")
endif()

#---- Geometry
add_executable(Geometry src/Geometry.cpp)
set_property(TARGET Geometry PROPERTY CXX_STANDARD 11)
target_include_directories(Geometry PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(Geometry osmscout)

#---- WorkQueue
add_executable(WorkQueue src/WorkQueue.cpp)
set_property(TARGET WorkQueue PROPERTY CXX_STANDARD 11)
target_include_directories(WorkQueue PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(WorkQueue osmscout)

#---- MapRotate
if(${OSMSCOUT_BUILD_MAP})
  add_executable(MapRotate src/MapRotate.cpp)
  set_property(TARGET MapRotate PROPERTY CXX_STANDARD 11)
  target_include_directories(MapRotate PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)
  target_link_libraries(MapRotate osmscout osmscout_map)
else()
	message("Skip MapRotate test libosmscout-map, is missing.")
endif()

#---- AccessParse
add_executable(AccessParse src/AccessParse.cpp)
set_property(TARGET AccessParse PROPERTY CXX_STANDARD 11)
target_include_directories(AccessParse PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(AccessParse osmscout)

#---- BitsAndBytesNeeded
add_executable(BitsAndBytesNeeded src/BitsAndBytesNeeded.cpp)
set_property(TARGET BitsAndBytesNeeded PROPERTY CXX_STANDARD 11)
target_include_directories(BitsAndBytesNeeded PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(BitsAndBytesNeeded osmscout)

#---- EncodeNumber
add_executable(EncodeNumber src/EncodeNumber.cpp)
set_property(TARGET EncodeNumber PROPERTY CXX_STANDARD 11)
target_include_directories(EncodeNumber PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(EncodeNumber osmscout)

#---- FileScannerWriter
add_executable(FileScannerWriter src/FileScannerWriter.cpp)
set_property(TARGET FileScannerWriter PROPERTY CXX_STANDARD 11)
target_include_directories(FileScannerWriter PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(FileScannerWriter osmscout)

#---- GeoCoordParse
add_executable(GeoCoordParse src/GeoCoordParse.cpp)
set_property(TARGET GeoCoordParse PROPERTY CXX_STANDARD 11)
target_include_directories(GeoCoordParse PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(GeoCoordParse osmscout)

#---- NumberSet
add_executable(NumberSet src/NumberSet.cpp)
set_property(TARGET NumberSet PROPERTY CXX_STANDARD 11)
target_include_directories(NumberSet PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(NumberSet osmscout)

#---- ScanConversion
add_executable(ScanConversion src/ScanConversion.cpp)
set_property(TARGET ScanConversion PROPERTY CXX_STANDARD 11)
target_include_directories(ScanConversion PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(ScanConversion osmscout)

#---- WStringStringConversion
add_executable(WStringStringConversion src/WStringStringConversion.cpp)
set_property(TARGET WStringStringConversion PROPERTY CXX_STANDARD 11)
target_include_directories(WStringStringConversion PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(WStringStringConversion osmscout)

