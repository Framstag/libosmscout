#---- AccessParse
#---- AccessParse
osmscout_test_project(NAME AccessParseTest SOURCES src/AccessParseTest.cpp)

#---- AsyncProcessing
osmscout_test_project(NAME AsyncProcessingTest SOURCES src/AsyncProcessingTest.cpp)

#---- AsyncWorker
osmscout_test_project(NAME AsyncWorkerTest SOURCES src/AsyncWorkerTest.cpp)

#---- Bearing
osmscout_test_project(NAME BearingTest SOURCES src/BearingTest.cpp)

#---- BitsAndBytesNeeded
osmscout_test_project(NAME BitsAndBytesNeededTest SOURCES src/BitsAndBytesNeededTest.cpp)

#---- ByteTest
osmscout_test_project(NAME ByteTest SOURCES src/ByteTest.cpp)

#---- CachePerformance
osmscout_test_project(NAME CachePerformanceTest SOURCES src/CachePerformanceTest.cpp COMMAND --size 1000)

#---- CalculateResolution
osmscout_test_project(NAME CalculateResolutionTest SOURCES src/CalculateResolutionTest.cpp)

#---- CancelableFuture
osmscout_test_project(NAME CancelableFutureTest SOURCES src/CancelableFutureTest.cpp)

#---- CmdLineParsing
osmscout_test_project(NAME CmdLineParsingTest SOURCES src/CmdLineParsingTest.cpp)

#---- ColorParse
osmscout_test_project(NAME ColorParseTest SOURCES src/ColorParseTest.cpp)

#---- CoordinateEncoding
osmscout_test_project(NAME CoordinateEncodingTest SOURCES src/CoordinateEncodingTest.cpp COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/data/testregion")

#---- FileFormatVersion
osmscout_test_project(NAME FileFormatVersionTest SOURCES src/FileFormatVersionTest.cpp)
set_tests_properties(FileFormatVersionTest PROPERTIES ENVIRONMENT "TESTS_TOP_DIR=${CMAKE_CURRENT_SOURCE_DIR}")

#---- Latch
osmscout_test_project(NAME LatchTest SOURCES src/LatchTest.cpp)

#---- LocationLookup
osmscout_test_project(NAME LocationLookupTest SOURCES src/LocationServiceTest.cpp src/SearchForLocationByStringTest.cpp src/SearchForLocationByFormTest.cpp src/SearchForPOIByFormTest.cpp TARGET OSMScout::Test OSMScout::Import)
set_source_files_properties(src/SearchForLocationByStringTest.cpp src/SearchForLocationByFormTest.cpp src/SearchForPOIByFormTest.cpp src/LocationServiceTest.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE)
set_tests_properties(LocationLookupTest PROPERTIES ENVIRONMENT "TESTS_TOP_DIR=${CMAKE_CURRENT_SOURCE_DIR};TESTS_TMP_DIR=${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(LocationLookupTest PROPERTIES UNITY_BUILD FALSE)

#---- NumberSetPerformance
osmscout_test_project(NAME NumberSetPerformanceTest SOURCES src/NumberSetPerformanceTest.cpp)

#---- OpeningHours
osmscout_test_project(NAME OpeningHoursTest SOURCES src/OpeningHoursTest.cpp)

#---- ReaderScannerPerformance
osmscout_test_project(NAME ReaderScannerPerformanceTest SOURCES src/ReaderScannerPerformanceTest.cpp COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/data/testregion")

#---- MultiDBRouting
osmscout_test_project(NAME MultiDBRoutingTest SOURCES src/MultiDBRoutingTest.cpp COMMAND 50.412 14.534 50.424 14.6013 "${CMAKE_CURRENT_SOURCE_DIR}/data/testregion")

#---- ThreadedDatabase
if(${OSMSCOUT_BUILD_MAP} AND TARGET OSMScout::Map)
	osmscout_test_project(NAME ThreadedDatabaseTest SOURCES src/ThreadedDatabaseTest.cpp TARGET OSMScout::Map COMMAND --threads 100 --iterations 1000 "${CMAKE_CURRENT_SOURCE_DIR}/data/testregion" "${CMAKE_CURRENT_SOURCE_DIR}/../stylesheets/standard.oss")
else()
	message("Skip ThreadedDatabase test, libosmscout-map is missing.")
endif()

#---- TextLookupTest
if(MARISA_FOUND)
	osmscout_demo_project(NAME TextLookupTest SOURCES src/TextLookupTest.cpp TARGET OSMScout::OSMScout OSMScout::OSMScout)

	add_test(NAME TextLookupTest-vysoka
		COMMAND TextLookupTest
		--expected-results 5
		"${CMAKE_CURRENT_SOURCE_DIR}/data/testregion"
		"vysoka"
		)

	add_test(NAME TextLookupTest-Vysoká
		COMMAND TextLookupTest
		--expected-results 5
		"${CMAKE_CURRENT_SOURCE_DIR}/data/testregion"
		"Vysoká"
		)

	add_test(NAME TextLookupTest-bosyne
		COMMAND TextLookupTest
		--expected-results 1
		"${CMAKE_CURRENT_SOURCE_DIR}/data/testregion"
		"bosyne"
		)
else()
	message("Skip TextLookupTest test, Marisa library is missing.")
endif()

#---- Thread
if(NOT MINGW AND NOT MSYS)
  osmscout_test_project(NAME ThreadTest SOURCES src/ThreadTest.cpp)
endif()

#---- DrawTextQt
if(${OSMSCOUT_BUILD_MAP_QT} AND TARGET OSMScout::MapQt)
	set(src_files src/DrawTextQt.cpp include/DrawWindow.h)
	qt_wrap_cpp(src_files include/DrawWindow.h)
	osmscout_test_project(NAME DrawTextQt SOURCES ${src_files} TARGET OSMScout::Map OSMScout::MapQt Qt::Widgets SKIPTEST)
else()
	message("Skip DrawTextQt test, libosmscout-map-qt is missing.")
endif()

#---- Geometry
osmscout_test_project(NAME GeometryTest SOURCES src/GeometryTest.cpp)

#---- HeaderCheck
osmscout_test_project(NAME HeaderCheckTest SOURCES src/HeaderCheckTest.cpp)
set_tests_properties(HeaderCheckTest PROPERTIES ENVIRONMENT "SOURCE_ROOT=${CMAKE_SOURCE_DIR}")

#---- WaterIndex
if(${OSMSCOUT_BUILD_IMPORT} AND TARGET OSMScout::Import)
	osmscout_test_project(NAME WaterIndexTest SOURCES src/WaterIndexTest.cpp TARGET OSMScout::Import)
else()
	message("Skip WaterIndex test, libosmscout-import is missing.")
endif()

#---- WorkQueue
osmscout_test_project(NAME WorkQueueTest SOURCES src/WorkQueueTest.cpp)

#---- MapRotate
if(NOT MINGW AND NOT MSYS)
	if(${OSMSCOUT_BUILD_MAP} AND TARGET OSMScout::Map)
		osmscout_test_project(NAME MapRotateTest SOURCES src/MapRotateTest.cpp TARGET OSMScout::Map)
	else()
		message("Skip MapRotate test, libosmscout-map is missing.")
	endif()
endif()

#---- EncodeNumber
osmscout_test_project(NAME EncodeNumberTest SOURCES src/EncodeNumberTest.cpp)

#---- FeatureLabelTest
osmscout_test_project(NAME FeatureLabelTest SOURCES src/FeatureLabelTest.cpp)

#---- File
osmscout_test_project(NAME FileTest SOURCES src/FileTest.cpp)

#---- FileScannerWriter
osmscout_test_project(NAME FileScannerWriterTest SOURCES src/FileScannerWriterTest.cpp)

#---- GeoCoordParse
osmscout_test_project(NAME GeoCoordParseTest SOURCES src/GeoCoordParseTest.cpp)

#---- NumberSet
osmscout_test_project(NAME NumberSetTest SOURCES src/NumberSetTest.cpp)

#---- ScanConversion
osmscout_test_project(NAME RoutePostprocessorTest SOURCES src/RoutePostprocessorTest.cpp)

#---- ScanConversion
osmscout_test_project(NAME ScanConversionTest SOURCES src/ScanConversionTest.cpp)

#---- ScreenMask
if(${OSMSCOUT_BUILD_MAP} AND TARGET OSMScout::Map)
  osmscout_test_project(NAME ScreenMaskTest SOURCES src/ScreenMaskTest.cpp TARGET OSMScout::Map)
else()
	message("Skip ScreenMask test, libosmscout-map is missing.")
endif()

#---- StringUtils
osmscout_test_project(NAME StringUtilsTest SOURCES src/StringUtilsTest.cpp)

#---- StdByteTest
osmscout_test_project(NAME StdByteTest SOURCES src/StdByteTest.cpp)

#---- Signal
osmscout_test_project(NAME SignalTest SOURCES src/SignalTest.cpp)

#---- TilingTest
osmscout_test_project(NAME TilingTest SOURCES src/TilingTest.cpp)

#---- TimeParse
osmscout_test_project(NAME TimeParseTest SOURCES src/TimeParseTest.cpp)

#---- WStringStringConversion
osmscout_test_project(NAME WStringStringConversionTest SOURCES src/WStringStringConversionTest.cpp)

#---- TransPolygon
osmscout_test_project(NAME TransPolygonTest SOURCES src/TransPolygonTest.cpp)

#---- GeoBox
osmscout_test_project(NAME GeoBoxTest SOURCES src/GeoBoxTest.cpp)

#---- OSTAndOSSTest
if(${OSMSCOUT_BUILD_MAP} AND TARGET OSMScout::Map)
	osmscout_test_project(NAME OSTAndOSSTest SOURCES src/OSTAndOSSTest.cpp TARGET OSMScout::Map SKIPTEST)

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
			COMMAND OSTAndOSSTest
			--warning-as-error
			${CMAKE_CURRENT_SOURCE_DIR}/../stylesheets/map.ost
			${CMAKE_CURRENT_SOURCE_DIR}/../stylesheets/${STYLESHEET})
	endforeach()
else()
    message("Skip OSTAndOSS test, libosmscout-map is missing.")
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
		target_link_libraries(PerformanceTest OSMScout::MapQt Qt::Widgets)
		set(HAVE_LIB_OSMSCOUTMAPQT 1)
		if(MSVC)
			visual_studio_qt_helper("PerformanceTest")
		endif()
		if(OSMSCOUT_INSTALL_QT_DLL AND TARGET Qt::windeployqt)
			add_custom_command(TARGET PerformanceTest
				POST_BUILD
				COMMAND set PATH=%PATH%$<SEMICOLON>${qt_install_prefix}/bin
				COMMAND Qt::windeployqt --dir "${CMAKE_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:PerformanceTest>/$<TARGET_FILE_NAME:PerformanceTest>"
				)
		endif()
		set(PERFORMANCE_TEST_DRIVERS ${PERFORMANCE_TEST_DRIVERS} Qt)
	endif()
	if(${OSMSCOUT_BUILD_MAP_AGG})
		target_link_libraries(PerformanceTest OSMScout::MapAGG)
		set(HAVE_LIB_OSMSCOUTMAPAGG 1)
		set(PERFORMANCE_TEST_DRIVERS ${PERFORMANCE_TEST_DRIVERS} agg)
	endif()
	if(${OSMSCOUT_BUILD_MAP_OPENGL} AND TARGET glfw)
		target_link_libraries(PerformanceTest OSMScout::MapOpenGL glfw)
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
osmscout_test_project(NAME Base64Test SOURCES src/Base64Test.cpp)

#---- CoordBufferTest
osmscout_test_project(NAME CoordBufferTest SOURCES src/CoordBufferTest.cpp)

#---- ClientQtThreading
if(${OSMSCOUT_BUILD_CLIENT_QT} AND TARGET OSMScout::MapQt AND TARGET OSMScout::ClientQt)
	set(src_files src/ClientQtThreading.cpp)
	qt_wrap_cpp(src_files include/ClientQtThreading.h)
	osmscout_test_project(NAME ClientQtThreading SOURCES ${src_files} TARGET OSMScout::Map OSMScout::MapQt OSMScout::ClientQt Qt::Widgets SKIPTEST)
else()
	message("Skip ClientQtThreading test, libosmscout-client-qt is missing.")
endif()

#---- QtFileDownloader
if(${OSMSCOUT_BUILD_CLIENT_QT} AND TARGET OSMScout::MapQt AND TARGET OSMScout::ClientQt)
	set(src_files src/QtFileDownloaderTest.cpp)
	qt_wrap_cpp(src_files include/DownloaderTest.h)
	osmscout_test_project(NAME QtFileDownloaderTest SOURCES ${src_files} TARGET OSMScout::Map OSMScout::MapQt OSMScout::ClientQt Qt::Widgets SKIPTEST)
else()
	message("Skip QtFileDownloader test, libosmscout-client-qt is missing.")
endif()

#---- ScreenBox
osmscout_test_project(NAME ScreenBoxTest SOURCES src/ScreenBoxTest.cpp)

#---- SunriseSunsetTest
osmscout_test_project(NAME SunriseSunsetTest SOURCES src/SunriseSunsetTest.cpp)

#---- PolygonCenterTest
osmscout_test_project(NAME PolygonCenterTest SOURCES src/PolygonCenterTest.cpp)

#---- MercatorProjection
osmscout_test_project(NAME MercatorProjectionTest SOURCES src/MercatorProjectionTest.cpp)

