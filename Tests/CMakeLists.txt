#---- AccessParse
osmscout_test_project(NAME AccessParse SOURCES src/AccessParse.cpp)

#---- AsyncProcessing
osmscout_test_project(NAME AsyncProcessing SOURCES src/AsyncProcessing.cpp)

#---- Bearing
osmscout_test_project(NAME Bearing SOURCES src/Bearing.cpp)

#---- BitsAndBytesNeeded
osmscout_test_project(NAME BitsAndBytesNeeded SOURCES src/BitsAndBytesNeeded.cpp)

#---- ByteTest
osmscout_test_project(NAME ByteTest SOURCES src/ByteTest.cpp)

#---- CachePerformance
osmscout_test_project(NAME CachePerformance SOURCES src/CachePerformance.cpp COMMAND --size 1000)

#---- CalculateResolution
osmscout_test_project(NAME CalculateResolution SOURCES src/CalculateResolution.cpp)

#---- CmdLineParsing
osmscout_test_project(NAME CmdLineParsing SOURCES src/CmdLineParsing.cpp)

#---- ColorParse
osmscout_test_project(NAME ColorParse SOURCES src/ColorParse.cpp)

#---- CoordinateEncoding
osmscout_test_project(NAME CoordinateEncoding SOURCES src/CoordinateEncoding.cpp COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/data/testregion")

#---- LocationLookup
osmscout_test_project(NAME LocationLookupTest SOURCES src/LocationServiceTest.cpp src/SearchForLocationByStringTest.cpp src/SearchForLocationByFormTest.cpp src/SearchForPOIByFormTest.cpp TARGET OSMScout::Test OSMScout::Import)
set_source_files_properties(src/SearchForLocationByStringTest.cpp src/SearchForLocationByFormTest.cpp src/SearchForPOIByFormTest.cpp src/LocationServiceTest.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE)
set_tests_properties(LocationLookupTest PROPERTIES ENVIRONMENT "TESTS_TOP_DIR=${CMAKE_CURRENT_SOURCE_DIR};TESTS_TMP_DIR=${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(LocationLookupTest PROPERTIES UNITY_BUILD FALSE)

#---- NumberSetPerformance
osmscout_test_project(NAME NumberSetPerformance SOURCES src/NumberSetPerformance.cpp)

#---- ReaderScannerPerformance
osmscout_test_project(NAME ReaderScannerPerformance SOURCES src/ReaderScannerPerformance.cpp COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/data/testregion")

#---- MultiDBRouting
osmscout_test_project(NAME MultiDBRouting SOURCES src/MultiDBRouting.cpp COMMAND 50.412 14.534 50.424 14.6013 "${CMAKE_CURRENT_SOURCE_DIR}/data/testregion")

#---- ThreadedDatabase
if(${OSMSCOUT_BUILD_MAP} AND TARGET OSMScout::Map)
	osmscout_test_project(NAME ThreadedDatabase SOURCES src/ThreadedDatabase.cpp TARGET OSMScout::Map COMMAND --threads 100 --iterations 1000 "${CMAKE_CURRENT_SOURCE_DIR}/data/testregion" "${CMAKE_CURRENT_SOURCE_DIR}/../stylesheets/standard.oss")
else()
	message("Skip ThreadedDatabase test, libosmscout-map is missing.")
endif()

#---- DrawTextQt
if(${OSMSCOUT_BUILD_MAP_QT} AND TARGET OSMScout::MapQt)
	set(src_files src/DrawTextQt.cpp include/DrawWindow.h)
	qt5_wrap_cpp(src_files include/DrawWindow.h)
	osmscout_test_project(NAME DrawTextQt SOURCES ${src_files} TARGET OSMScout::Map OSMScout::MapQt Qt5::Widgets SKIPTEST)
else()
	message("Skip DrawTextQt test, libosmscout-map-qt is missing.")
endif()

#---- Geometry
osmscout_test_project(NAME Geometry SOURCES src/Geometry.cpp)

#---- WorkQueue
osmscout_test_project(NAME WorkQueue SOURCES src/WorkQueue.cpp)

#---- MapRotate
if(NOT MINGW AND NOT MSYS)
	if(${OSMSCOUT_BUILD_MAP} AND TARGET OSMScout::Map)
		osmscout_test_project(NAME MapRotate SOURCES src/MapRotate.cpp TARGET OSMScout::Map)
	else()
		message("Skip MapRotate test, libosmscout-map is missing.")
	endif()
endif()

#---- EncodeNumber
osmscout_test_project(NAME EncodeNumber SOURCES src/EncodeNumber.cpp)

#---- FeatureLabelTest
osmscout_test_project(NAME FeatureLabelTest SOURCES src/FeatureLabelTest.cpp)

#---- File
osmscout_test_project(NAME File SOURCES src/File.cpp)

#---- FileScannerWriter
osmscout_test_project(NAME FileScannerWriter SOURCES src/FileScannerWriter.cpp)

#---- GeoCoordParse
osmscout_test_project(NAME GeoCoordParse SOURCES src/GeoCoordParse.cpp)

#---- NumberSet
osmscout_test_project(NAME NumberSet SOURCES src/NumberSet.cpp)

#---- ScanConversion
osmscout_test_project(NAME ScanConversion SOURCES src/ScanConversion.cpp)

#---- StringUtils
osmscout_test_project(NAME StringUtils SOURCES src/StringUtils.cpp)

#---- StdByteTest
osmscout_test_project(NAME StdByteTest SOURCES src/StdByteTest.cpp)

#---- TilingTest
osmscout_test_project(NAME TilingTest SOURCES src/TilingTest.cpp)

#---- TimeParse
osmscout_test_project(NAME TimeParse SOURCES src/TimeParse.cpp)

#---- WStringStringConversion
osmscout_test_project(NAME WStringStringConversion SOURCES src/WStringStringConversion.cpp)

#---- TransPolygon
osmscout_test_project(NAME TransPolygon SOURCES src/TransPolygon.cpp)

#---- GeoBox
osmscout_test_project(NAME GeoBox SOURCES src/GeoBox.cpp)

#---- OSTAndOSSCheck
if(${OSMSCOUT_BUILD_MAP} AND TARGET OSMScout::Map)
	osmscout_test_project(NAME OSTAndOSSCheck SOURCES src/OSTAndOSSCheck.cpp TARGET OSMScout::Map SKIPTEST)

	set(STYLESHEETS
		standard.oss
		winter-sports.oss
		boundaries.oss
		railways.oss
		motorways.oss
		public-transport.oss
		cycle.oss)

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

#---- PerformanceTest
if(${OSMSCOUT_BUILD_MAP})
	osmscout_demo_project(NAME PerformanceTest SOURCES src/PerformanceTest.cpp TARGET OSMScout::OSMScout OSMScout::Map)
	target_include_directories(PerformanceTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/PerformanceTest)

	set(PERFORMANCE_TEST_DRIVERS noop none)

	if(${OSMSCOUT_BUILD_MAP_CAIRO})
		target_link_libraries(PerformanceTest OSMScout::MapCairo)
		set(HAVE_LIB_OSMSCOUTMAPCAIRO 1)
		set(PERFORMANCE_TEST_DRIVERS ${PERFORMANCE_TEST_DRIVERS} cairo)
	endif()
	if(${OSMSCOUT_BUILD_MAP_QT})
		target_link_libraries(PerformanceTest OSMScout::MapQt Qt5::Widgets)
		set(HAVE_LIB_OSMSCOUTMAPQT 1)
		if(MSVC)
			visual_studio_qt_helper("PerformanceTest")
		endif()
		if(OSMSCOUT_INSTALL_QT_DLL AND TARGET Qt5::windeployqt)
			add_custom_command(TARGET PerformanceTest
				POST_BUILD
				COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
				COMMAND Qt5::windeployqt --dir "${CMAKE_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:PerformanceTest>/$<TARGET_FILE_NAME:PerformanceTest>"
				)
		endif()
		set(PERFORMANCE_TEST_DRIVERS ${PERFORMANCE_TEST_DRIVERS} Qt)
	endif()
	if(${OSMSCOUT_BUILD_MAP_AGG})
		target_link_libraries(PerformanceTest OSMScout::MapAGG)
		set(HAVE_LIB_OSMSCOUTMAPAGG 1)
		set(PERFORMANCE_TEST_DRIVERS ${PERFORMANCE_TEST_DRIVERS} agg)
	endif()
	if(${OSMSCOUT_BUILD_MAP_OPENGL} AND ${GLFW_FOUND})
		target_include_directories(PerformanceTest PRIVATE ${GLFW_INCLUDE_DIR})
		target_link_libraries(PerformanceTest OSMScout::MapOpenGL ${GLFW_LIBRARIES})
		set(HAVE_LIB_OSMSCOUTMAPOPENGL 1)
		set(PERFORMANCE_TEST_DRIVERS ${PERFORMANCE_TEST_DRIVERS} opengl)
	endif()
	if(${OSMSCOUT_BUILD_MAP_GDI})
		target_link_libraries(PerformanceTest OSMScout::MapGDI)
		set(HAVE_LIB_OSMSCOUTMAPGDI 1)
		set(PERFORMANCE_TEST_DRIVERS ${PERFORMANCE_TEST_DRIVERS} dgi)
	endif()

	option(PERF_TEST_GPERFTOOLS_USAGE "Enable Gperftools in Performance test" ${GPERFTOOLS_USAGE})
	if(${PERF_TEST_GPERFTOOLS_USAGE})
		target_include_directories(PerformanceTest PRIVATE ${GPERFTOOLS_INCLUDE_DIRS})
		target_link_libraries(PerformanceTest ${GPERFTOOLS_LIBRARIES})
	endif()
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/PerformanceTestConfig.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/PerformanceTest/config.h)

	set(STYLESHEETS
		standard.oss
		winter-sports.oss
		boundaries.oss
		railways.oss
		motorways.oss
		public-transport.oss
		cycle.oss)

	foreach(DRIVER ${PERFORMANCE_TEST_DRIVERS})
		foreach(STYLESHEET ${STYLESHEETS})
			add_test(NAME PerformanceTest-${DRIVER}-${STYLESHEET}
				COMMAND PerformanceTest
				--debug
				--driver ${DRIVER}
				--icons "${CMAKE_CURRENT_SOURCE_DIR}/../libosmscout/data/icons/svg/standard"
				--font "${CMAKE_CURRENT_SOURCE_DIR}/../libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf"
				"${CMAKE_CURRENT_SOURCE_DIR}/../Tests/data/testregion"
				"${CMAKE_CURRENT_SOURCE_DIR}/../stylesheets/${STYLESHEET}"
				50.3806 14.5418 50.3806 14.5418)
		endforeach()
	endforeach()

else()
	message("Skip PerformanceTest test, libosmscout-map is missing.")
endif()

#---- LabelPathTest
if(${OSMSCOUT_BUILD_MAP} AND TARGET OSMScout::Map)
	osmscout_test_project(NAME LabelPathTest SOURCES src/LabelPathTest.cpp TARGET OSMScout::Map)
else()
	message("Skip LabelPathTest, libosmscout-map is missing.")
endif()

#---- Base64
osmscout_test_project(NAME Base64 SOURCES src/Base64.cpp)

#---- CoordBufferTest
osmscout_test_project(NAME CoordBufferTest SOURCES src/CoordBufferTest.cpp)

#---- ClientQtThreading
if(${OSMSCOUT_BUILD_CLIENT_QT} AND TARGET OSMScout::MapQt AND TARGET OSMScout::ClientQt)
	set(src_files src/ClientQtThreading.cpp)
	qt5_wrap_cpp(src_files include/ClientQtThreading.h)
	osmscout_test_project(NAME ClientQtThreading SOURCES ${src_files} TARGET OSMScout::Map OSMScout::MapQt OSMScout::ClientQt Qt5::Widgets SKIPTEST)
else()
	message("Skip ClientQtThreading test, libosmscout-client-qt is missing.")
endif()

#---- QtFileDownloader
if(${OSMSCOUT_BUILD_CLIENT_QT} AND TARGET OSMScout::MapQt AND TARGET OSMScout::ClientQt)
	set(src_files src/QtFileDownloader.cpp)
	qt5_wrap_cpp(src_files include/DownloaderTest.h)
	osmscout_test_project(NAME QtFileDownloader SOURCES ${src_files} TARGET OSMScout::Map OSMScout::MapQt OSMScout::ClientQt Qt5::Widgets SKIPTEST)
else()
	message("Skip QtFileDownloader test, libosmscout-client-qt is missing.")
endif()

#---- SunriseSunsetTest
osmscout_test_project(NAME SunriseSunsetTest SOURCES src/SunriseSunsetTest.cpp)

#---- PolygonCenterTest
osmscout_test_project(NAME PolygonCenterTest SOURCES src/PolygonCenter.cpp)
