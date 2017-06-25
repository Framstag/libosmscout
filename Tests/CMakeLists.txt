#---- AccessParse
add_executable(AccessParse src/AccessParse.cpp)
set_property(TARGET AccessParse PROPERTY CXX_STANDARD 11)
target_include_directories(AccessParse PRIVATE include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(AccessParse osmscout)
add_test(NAME AccessParse COMMAND AccessParse)

#---- Bearing
add_executable(Bearing src/Bearing.cpp)
set_property(TARGET Bearing PROPERTY CXX_STANDARD 11)
target_include_directories(Bearing PRIVATE include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(Bearing osmscout)
add_test(NAME Bearing COMMAND Bearing)

#---- BitsAndBytesNeeded
add_executable(BitsAndBytesNeeded src/BitsAndBytesNeeded.cpp)
set_property(TARGET BitsAndBytesNeeded PROPERTY CXX_STANDARD 11)
target_include_directories(BitsAndBytesNeeded PRIVATE include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(BitsAndBytesNeeded osmscout)
add_test(NAME BitsAndBytesNeeded COMMAND BitsAndBytesNeeded)

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
add_test(NAME CalculateResolution COMMAND CalculateResolution)

#---- CmdLineParsing
add_executable(CmdLineParsing src/CmdLineParsing.cpp)
set_property(TARGET CmdLineParsing PROPERTY CXX_STANDARD 11)
target_include_directories(CmdLineParsing PRIVATE include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CmdLineParsing osmscout)
add_test(NAME CmdLineParsing COMMAND CmdLineParsing)

#---- ColorParse
add_executable(ColorParse src/ColorParse.cpp)
set_property(TARGET ColorParse PROPERTY CXX_STANDARD 11)
target_include_directories(ColorParse PRIVATE include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(ColorParse osmscout)
add_test(NAME ColorParse COMMAND ColorParse)

#---- CoordinateCalculations
add_executable(CoordinateCalculations src/CoordinateCalculations.cpp)
set_property(TARGET CoordinateCalculations PROPERTY CXX_STANDARD 11)
target_include_directories(CoordinateCalculations PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CoordinateCalculations osmscout)
add_test(NAME CoordinateCalculations COMMAND CoordinateCalculations)

#---- CoordinateEncoding
add_executable(CoordinateEncoding src/CoordinateEncoding.cpp)
set_property(TARGET CoordinateEncoding PROPERTY CXX_STANDARD 11)
target_include_directories(CoordinateEncoding PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CoordinateEncoding osmscout)
# TODO: add sample data and arguments to test
#add_test(NAME CoordinateEncoding COMMAND CoordinateEncoding)

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
# TODO: add sample data and arguments to test
#add_test(NAME  COMMAND )

#---- ThreadedDatabase
if(${OSMSCOUT_BUILD_MAP})
	add_executable(ThreadedDatabase src/ThreadedDatabase.cpp)
	set_property(TARGET ThreadedDatabase PROPERTY CXX_STANDARD 11)
	target_include_directories(ThreadedDatabase PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)
	target_link_libraries(ThreadedDatabase osmscout osmscout_map)
  # TODO: add sample data and arguments to test
  #add_test(NAME  COMMAND )
else()
	message("Skip ThreadedDatabase test libosmscout-map, is missing.")
endif()


if(${OSMSCOUT_BUILD_MAP_QT})
  set(src_files src/DrawTextQt.cpp include/DrawWindow.h)
  qt5_wrap_cpp(src_files include/DrawWindow.h)
  add_executable(DrawTextQt ${src_files})
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
add_test(NAME Geometry COMMAND Geometry)

#---- WorkQueue
add_executable(WorkQueue src/WorkQueue.cpp)
set_property(TARGET WorkQueue PROPERTY CXX_STANDARD 11)
target_include_directories(WorkQueue PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(WorkQueue osmscout)
add_test(NAME WorkQueue COMMAND WorkQueue)

#---- MapRotate
if(${OSMSCOUT_BUILD_MAP})
  add_executable(MapRotate src/MapRotate.cpp)
  set_property(TARGET MapRotate PROPERTY CXX_STANDARD 11)
  target_include_directories(MapRotate PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)
  target_link_libraries(MapRotate osmscout osmscout_map)
  add_test(NAME MapRotate COMMAND MapRotate)
else()
	message("Skip MapRotate test libosmscout-map, is missing.")
endif()

#---- EncodeNumber
add_executable(EncodeNumber src/EncodeNumber.cpp)
set_property(TARGET EncodeNumber PROPERTY CXX_STANDARD 11)
target_include_directories(EncodeNumber PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(EncodeNumber osmscout)
add_test(NAME EncodeNumber COMMAND EncodeNumber)

#---- FileScannerWriter
add_executable(FileScannerWriter src/FileScannerWriter.cpp)
set_property(TARGET FileScannerWriter PROPERTY CXX_STANDARD 11)
target_include_directories(FileScannerWriter PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(FileScannerWriter osmscout)
add_test(NAME FileScannerWriter COMMAND FileScannerWriter)

#---- GeoCoordParse
add_executable(GeoCoordParse src/GeoCoordParse.cpp)
set_property(TARGET GeoCoordParse PROPERTY CXX_STANDARD 11)
target_include_directories(GeoCoordParse PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(GeoCoordParse osmscout)
add_test(NAME GeoCoordParse COMMAND GeoCoordParse)

#---- NumberSet
add_executable(NumberSet src/NumberSet.cpp)
set_property(TARGET NumberSet PROPERTY CXX_STANDARD 11)
target_include_directories(NumberSet PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(NumberSet osmscout)
add_test(NAME NumberSet COMMAND NumberSet)

#---- ScanConversion
add_executable(ScanConversion src/ScanConversion.cpp)
set_property(TARGET ScanConversion PROPERTY CXX_STANDARD 11)
target_include_directories(ScanConversion PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(ScanConversion osmscout)
add_test(NAME ScanConversion COMMAND ScanConversion)

#---- WStringStringConversion
add_executable(WStringStringConversion src/WStringStringConversion.cpp)
set_property(TARGET WStringStringConversion PROPERTY CXX_STANDARD 11)
target_include_directories(WStringStringConversion PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(WStringStringConversion osmscout)
add_test(NAME WStringStringConversion COMMAND WStringStringConversion)

#---- TransPolygon
add_executable(TransPolygon src/TransPolygon.cpp include/TestWay.h)
set_property(TARGET TransPolygon PROPERTY CXX_STANDARD 11)
target_include_directories(TransPolygon PRIVATE include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(TransPolygon osmscout)
add_test(NAME TransPolygon COMMAND TransPolygon)

#---- TransPolygon
add_executable(GeoBox src/GeoBox.cpp)
set_property(TARGET GeoBox PROPERTY CXX_STANDARD 11)
target_include_directories(GeoBox PRIVATE include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(GeoBox osmscout)
add_test(NAME GeoBox COMMAND GeoBox)

#---- OSTAndOSSCheck
if(${OSMSCOUT_BUILD_MAP})
  add_executable(OSTAndOSSCheck src/OSTAndOSSCheck.cpp)
  set_property(TARGET OSTAndOSSCheck PROPERTY CXX_STANDARD 11)
  target_include_directories(OSTAndOSSCheck PRIVATE include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)
  target_link_libraries(OSTAndOSSCheck osmscout osmscout_map)
  add_test(NAME OSTAndOSSCheck COMMAND OSTAndOSSCheck)
  set_tests_properties(OSTAndOSSCheck PROPERTIES ENVIRONMENT TESTS_TOP_DIR=${CMAKE_CURRENT_SOURCE_DIR})
else()
    message("Skip MapRotate test libosmscout-map, is missing.")
endif()
