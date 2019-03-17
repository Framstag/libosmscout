#---- AccessParse
add_executable(AccessParse src/AccessParse.cpp)
set_property(TARGET AccessParse PROPERTY CXX_STANDARD 11)
target_include_directories(AccessParse PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(AccessParse OSMScout)
add_test(NAME AccessParse COMMAND AccessParse)

#---- Bearing
add_executable(Bearing src/Bearing.cpp)
set_property(TARGET Bearing PROPERTY CXX_STANDARD 11)
target_include_directories(Bearing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(Bearing OSMScout)
add_test(NAME Bearing COMMAND Bearing)

#---- BitsAndBytesNeeded
add_executable(BitsAndBytesNeeded src/BitsAndBytesNeeded.cpp)
set_property(TARGET BitsAndBytesNeeded PROPERTY CXX_STANDARD 11)
target_include_directories(BitsAndBytesNeeded PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(BitsAndBytesNeeded OSMScout)
add_test(NAME BitsAndBytesNeeded COMMAND BitsAndBytesNeeded)

#---- CachePerformance
add_executable(CachePerformance src/CachePerformance.cpp)
set_property(TARGET CachePerformance PROPERTY CXX_STANDARD 11)
target_link_libraries(CachePerformance OSMScout)

#---- CalculateResolution
add_executable(CalculateResolution src/CalculateResolution.cpp)
set_property(TARGET CalculateResolution PROPERTY CXX_STANDARD 11)
target_link_libraries(CalculateResolution OSMScout)

#---- CmdLineParsing
add_executable(CmdLineParsing src/CmdLineParsing.cpp)
set_property(TARGET CmdLineParsing PROPERTY CXX_STANDARD 11)
target_include_directories(CmdLineParsing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(CmdLineParsing OSMScout)
add_test(NAME CmdLineParsing COMMAND CmdLineParsing)

#---- ColorParse
add_executable(ColorParse src/ColorParse.cpp)
set_property(TARGET ColorParse PROPERTY CXX_STANDARD 11)
target_include_directories(ColorParse PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ColorParse OSMScout)
add_test(NAME ColorParse COMMAND ColorParse)

#---- CoordinateEncoding
add_executable(CoordinateEncoding src/CoordinateEncoding.cpp)
set_property(TARGET CoordinateEncoding PROPERTY CXX_STANDARD 11)
target_link_libraries(CoordinateEncoding OSMScout)
# TODO: add sample data and arguments to test
#add_test(NAME CoordinateEncoding COMMAND CoordinateEncoding)

#---- LocationLookup
add_executable(LocationLookupTest src/SearchForLocationByStringTest.cpp src/SearchForLocationByFormTest.cpp src/SearchForPOIByFormTest.cpp src/LocationServiceTest.cpp)
target_include_directories(LocationLookupTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_property(TARGET LocationLookupTest PROPERTY CXX_STANDARD 11)
target_link_libraries(LocationLookupTest OSMScoutTest OSMScoutImport OSMScout)
add_test(NAME LocationLookupTest COMMAND LocationLookupTest)
set_tests_properties(LocationLookupTest PROPERTIES ENVIRONMENT TESTS_TOP_DIR=${CMAKE_CURRENT_SOURCE_DIR})

#---- NumberSetPerformance
add_executable(NumberSetPerformance src/NumberSetPerformance.cpp)
set_property(TARGET NumberSetPerformance PROPERTY CXX_STANDARD 11)
target_link_libraries(NumberSetPerformance OSMScout)

#---- ReaderScannerPerformance
add_executable(ReaderScannerPerformance src/ReaderScannerPerformance.cpp)
set_property(TARGET ReaderScannerPerformance PROPERTY CXX_STANDARD 11)
target_link_libraries(ReaderScannerPerformance OSMScout)

#---- MultiDBRouting
add_executable(MultiDBRouting src/MultiDBRouting.cpp)
set_property(TARGET MultiDBRouting PROPERTY CXX_STANDARD 11)
target_link_libraries(MultiDBRouting OSMScout)
# TODO: add sample data and arguments to test
#add_test(NAME  COMMAND )

#---- ThreadedDatabase
if(${OSMSCOUT_BUILD_MAP})
	add_executable(ThreadedDatabase src/ThreadedDatabase.cpp)
	set_property(TARGET ThreadedDatabase PROPERTY CXX_STANDARD 11)
    target_link_libraries(ThreadedDatabase OSMScout OSMScoutMap)
  # TODO: add sample data and arguments to test
  #add_test(NAME  COMMAND )
else()
	message("Skip ThreadedDatabase test, libosmscout-map is missing.")
endif()


if(${OSMSCOUT_BUILD_MAP_QT})
  set(src_files src/DrawTextQt.cpp include/DrawWindow.h)
  qt5_wrap_cpp(src_files include/DrawWindow.h)
  add_executable(DrawTextQt ${src_files})
  set_property(TARGET DrawTextQt PROPERTY CXX_STANDARD 11)
  target_include_directories(DrawTextQt PRIVATE include ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(DrawTextQt OSMScout OSMScoutMap OSMScoutMapQt Qt5::Widgets)
else()
  message("Skip DrawTextQt test, libosmscout-map-qt is missing.")
endif()

#---- Geometry
add_executable(Geometry src/Geometry.cpp)
set_property(TARGET Geometry PROPERTY CXX_STANDARD 11)
target_link_libraries(Geometry OSMScout)
target_include_directories(Geometry PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME Geometry COMMAND Geometry)

#---- WorkQueue
add_executable(WorkQueue src/WorkQueue.cpp)
set_property(TARGET WorkQueue PROPERTY CXX_STANDARD 11)
target_link_libraries(WorkQueue OSMScout)
add_test(NAME WorkQueue COMMAND WorkQueue)

#---- MapRotate
if(${OSMSCOUT_BUILD_MAP})
  add_executable(MapRotate src/MapRotate.cpp)
  set_property(TARGET MapRotate PROPERTY CXX_STANDARD 11)
  target_link_libraries(MapRotate OSMScout OSMScoutMap)
  add_test(NAME MapRotate COMMAND MapRotate)
else()
	message("Skip MapRotate test, libosmscout-map is missing.")
endif()

#---- EncodeNumber
add_executable(EncodeNumber src/EncodeNumber.cpp)
set_property(TARGET EncodeNumber PROPERTY CXX_STANDARD 11)
target_link_libraries(EncodeNumber OSMScout)
add_test(NAME EncodeNumber COMMAND EncodeNumber)

#---- FileScannerWriter
add_executable(FileScannerWriter src/FileScannerWriter.cpp)
set_property(TARGET FileScannerWriter PROPERTY CXX_STANDARD 11)
target_link_libraries(FileScannerWriter OSMScout)
add_test(NAME FileScannerWriter COMMAND FileScannerWriter)

#---- GeoCoordParse
add_executable(GeoCoordParse src/GeoCoordParse.cpp)
set_property(TARGET GeoCoordParse PROPERTY CXX_STANDARD 11)
target_link_libraries(GeoCoordParse OSMScout)
add_test(NAME GeoCoordParse COMMAND GeoCoordParse)

#---- NumberSet
add_executable(NumberSet src/NumberSet.cpp)
set_property(TARGET NumberSet PROPERTY CXX_STANDARD 11)
target_link_libraries(NumberSet OSMScout)
add_test(NAME NumberSet COMMAND NumberSet)

#---- ScanConversion
add_executable(ScanConversion src/ScanConversion.cpp)
set_property(TARGET ScanConversion PROPERTY CXX_STANDARD 11)
target_include_directories(ScanConversion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ScanConversion OSMScout)
add_test(NAME ScanConversion COMMAND ScanConversion)

#---- TilingTest
add_executable(TilingTest src/TilingTest.cpp)
set_property(TARGET TilingTest PROPERTY CXX_STANDARD 11)
target_include_directories(TilingTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(TilingTest OSMScout)
add_test(NAME TilingTest COMMAND TilingTest)

#---- TimeParse
add_executable(TimeParse src/TimeParse.cpp)
set_property(TARGET TimeParse PROPERTY CXX_STANDARD 11)
target_include_directories(TimeParse PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(TimeParse OSMScout)
add_test(NAME TimeParse COMMAND TimeParse)

#---- WStringStringConversion
add_executable(WStringStringConversion src/WStringStringConversion.cpp)
set_property(TARGET WStringStringConversion PROPERTY CXX_STANDARD 11)
target_link_libraries(WStringStringConversion OSMScout)
add_test(NAME WStringStringConversion COMMAND WStringStringConversion)

#---- TransPolygon
add_executable(TransPolygon src/TransPolygon.cpp include/TestWay.h)
set_property(TARGET TransPolygon PROPERTY CXX_STANDARD 11)
target_include_directories(TransPolygon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(TransPolygon OSMScout)
add_test(NAME TransPolygon COMMAND TransPolygon)

#---- TransPolygon
add_executable(GeoBox src/GeoBox.cpp)
set_property(TARGET GeoBox PROPERTY CXX_STANDARD 11)
target_link_libraries(GeoBox OSMScout)
target_include_directories(GeoBox PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME GeoBox COMMAND GeoBox)

#---- OSTAndOSSCheck
if(${OSMSCOUT_BUILD_MAP})
  add_executable(OSTAndOSSCheck src/OSTAndOSSCheck.cpp)
  set_property(TARGET OSTAndOSSCheck PROPERTY CXX_STANDARD 11)
  target_link_libraries(OSTAndOSSCheck OSMScout OSMScoutMap)

  set(STYLESHEETS
          standard.oss
          winter-sports.oss
          boundaries.oss
          railways.oss
          motorways.oss
          public-transport.oss)

  foreach(STYLESHEET ${STYLESHEETS})
    add_test(NAME CheckStyleSheet-${STYLESHEET}
            COMMAND OSTAndOSSCheck
              --warning-as-error
              ${CMAKE_CURRENT_SOURCE_DIR}/../stylesheets/map.ost
              ${CMAKE_CURRENT_SOURCE_DIR}/../stylesheets/${STYLESHEET})
  endforeach()
else()
    message("Skip OSTAndOSSCheck test, libosmscout-map is missing.")
endif()

#---- LabelPathTest
if(${OSMSCOUT_BUILD_MAP})
  add_executable(LabelPathTest src/LabelPathTest.cpp)
  set_property(TARGET LabelPathTest PROPERTY CXX_STANDARD 11)
  target_include_directories(LabelPathTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(LabelPathTest OSMScout OSMScoutMap)
  add_test(NAME LabelPathTest COMMAND LabelPathTest)
else()
  message("Skip LabelPathTest, libosmscout-map is missing.")
endif()

#---- Base64
add_executable(Base64 src/Base64.cpp)
set_property(TARGET Base64 PROPERTY CXX_STANDARD 11)
target_link_libraries(Base64 OSMScout)
target_include_directories(Base64 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME Base64 COMMAND Base64)

#---- CoordBufferTest
add_executable(CoordBufferTest src/CoordBufferTest.cpp)
set_property(TARGET CoordBufferTest PROPERTY CXX_STANDARD 11)
target_link_libraries(CoordBufferTest OSMScout OSMScoutMap)
target_include_directories(CoordBufferTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME CoordBufferTest COMMAND CoordBufferTest)


if(${OSMSCOUT_BUILD_CLIENT_QT})
  set(src_files src/ClientQtThreading.cpp)
  qt5_wrap_cpp(src_files include/ClientQtThreading.h)
  add_executable(ClientQtThreading ${src_files})
  set_property(TARGET ClientQtThreading PROPERTY CXX_STANDARD 11)
  target_include_directories(ClientQtThreading PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(ClientQtThreading OSMScout OSMScoutMap OSMScoutMapQt OSMScoutClientQt Qt5::Widgets)
else()
  message("Skip ClientQtThreading test, libosmscout-client-qt is missing.")
endif()
