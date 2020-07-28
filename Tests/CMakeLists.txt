macro(create_test_project)
	set(_name)
	set(_sources)
	set(_targets OSMScout::OSMScout)
	set(_includes
		include
		${CMAKE_SOURCE_DIR}/include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${CMAKE_CURRENT_BINARY_DIR}/privateinclude
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/include
	)
	if(MARISA_FOUND)
		list(APPEND _includes ${MARISA_INCLUDE_DIRS})
	endif()

	set(_command)
	set(_skiptest OFF)
	set(cmd "_name")
	foreach(arg ${ARGN})
		if(arg STREQUAL "NAME")
			set(cmd "_name")
		elseif(arg STREQUAL "SOURCES")
			set(cmd "_sources")
		elseif(arg STREQUAL "TARGET")
			set(cmd "_targets")
		elseif(arg STREQUAL "INCLUDES")
			set(cmd "_includes")
		elseif(arg STREQUAL "COMMAND")
			set(cmd "_command")
		elseif(arg STREQUAL "SKIPTEST")
			set(_skiptest ON)
		else()
			if("${cmd}" STREQUAL "_name")
				set(_name ${arg})
			else()
				list(APPEND ${cmd} ${arg})
			endif()
		endif()
	endforeach()
	add_executable(${_name} ${_sources})
	set_target_properties(${_name} PROPERTIES VERSION ${OSMSCOUT_LIBRARY_VERSION} SOVERSION ${OSMSCOUT_LIBRARY_VERSION} FOLDER "Tests")
	target_link_libraries(${_name} ${_targets})
	install(TARGETS ${_name} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
	target_include_directories(${_name} PRIVATE ${_includes})
	if(WIN32)
		create_win32_test_resource(${_name})
	endif()
	if(NOT _skiptest)
		add_test(NAME ${_name} COMMAND ${_name} ${_command} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
		if(MSVC)
			set(qt5_bin "")
			if(Qt5_FOUND AND MSVC AND TARGET Qt5::qmake)
				get_target_property(_qt5_qmake_location Qt5::qmake IMPORTED_LOCATION)
				execute_process(COMMAND "${_qt5_qmake_location}" -query QT_INSTALL_PREFIX RESULT_VARIABLE return_code OUTPUT_VARIABLE qt5_install_prefix OUTPUT_STRIP_TRAILING_WHITESPACE)
				set(qt5_bin ";${qt5_install_prefix}\\bin")
			endif()
			set(envpath "PATH=$<TARGET_FILE_DIR:OSMScout>${qt5_bin}")
			foreach(tg IN LISTS OSMScoutMap OSMScoutMapAGG OSMScoutMapQt OSMScoutMapCairo OSMScoutImport OSMScoutGPX OSMScoutClientQt OSMScoutTest)
				if(TARGET ${tg})
					set(envpath "${envpath};$<TARGET_FILE_DIR:${tg}>")
				endif()
			endforeach()
			set_tests_properties(${_name} PROPERTIES ENVIRONMENT "${envpath};$ENV{PATH}")
		endif()
	endif()
endmacro(create_test_project)

#---- AccessParse
create_test_project(NAME AccessParse SOURCES src/AccessParse.cpp)

#---- Bearing
create_test_project(NAME Bearing SOURCES src/Bearing.cpp)

#---- BitsAndBytesNeeded
create_test_project(NAME BitsAndBytesNeeded SOURCES src/BitsAndBytesNeeded.cpp)

#---- CachePerformance
create_test_project(NAME CachePerformance SOURCES src/CachePerformance.cpp COMMAND --size 1000)

#---- CalculateResolution
create_test_project(NAME CalculateResolution SOURCES src/CalculateResolution.cpp)

#---- CmdLineParsing
create_test_project(NAME CmdLineParsing SOURCES src/CmdLineParsing.cpp)

#---- ColorParse
create_test_project(NAME ColorParse SOURCES src/ColorParse.cpp)

#---- CoordinateEncoding
create_test_project(NAME CoordinateEncoding SOURCES src/CoordinateEncoding.cpp COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/data/testregion")

#---- LocationLookup
create_test_project(NAME LocationLookupTest SOURCES src/SearchForLocationByStringTest.cpp src/SearchForLocationByFormTest.cpp src/SearchForPOIByFormTest.cpp src/LocationServiceTest.cpp TARGET OSMScout::Test OSMScout::Import)
set_tests_properties(LocationLookupTest PROPERTIES ENVIRONMENT TESTS_TOP_DIR=${CMAKE_CURRENT_SOURCE_DIR})

#---- NumberSetPerformance
create_test_project(NAME NumberSetPerformance SOURCES src/NumberSetPerformance.cpp)

#---- ReaderScannerPerformance
create_test_project(NAME ReaderScannerPerformance SOURCES src/ReaderScannerPerformance.cpp)

#---- MultiDBRouting
create_test_project(NAME MultiDBRouting SOURCES src/MultiDBRouting.cpp COMMAND 50.412 14.534 50.424 14.6013 "${CMAKE_CURRENT_SOURCE_DIR}/data/testregion")

#---- ThreadedDatabase
if(${OSMSCOUT_BUILD_MAP} AND TARGET OSMScout::Map)
	create_test_project(NAME ThreadedDatabase SOURCES src/ThreadedDatabase.cpp TARGET OSMScout::Map COMMAND --threads 100 --iterations 1000 "${CMAKE_CURRENT_SOURCE_DIR}/data/testregion" "${CMAKE_CURRENT_SOURCE_DIR}/../stylesheets/standard.oss")
else()
	message("Skip ThreadedDatabase test, libosmscout-map is missing.")
endif()

#---- DrawTextQt
if(${OSMSCOUT_BUILD_MAP_QT} AND TARGET OSMScout::MapQt)
	set(src_files src/DrawTextQt.cpp include/DrawWindow.h)
	qt5_wrap_cpp(src_files include/DrawWindow.h)
	create_test_project(NAME DrawTextQt SOURCES ${src_files} TARGET OSMScout::Map OSMScout::MapQt Qt5::Widgets SKIPTEST)
else()
	message("Skip DrawTextQt test, libosmscout-map-qt is missing.")
endif()

#---- Geometry
create_test_project(NAME Geometry SOURCES src/Geometry.cpp)

#---- WorkQueue
create_test_project(NAME WorkQueue SOURCES src/WorkQueue.cpp)

#---- MapRotate
if(${OSMSCOUT_BUILD_MAP} AND TARGET OSMScout::Map)
	create_test_project(NAME MapRotate SOURCES src/MapRotate.cpp TARGET OSMScout::Map)
else()
	message("Skip MapRotate test, libosmscout-map is missing.")
endif()

#---- EncodeNumber
create_test_project(NAME EncodeNumber SOURCES src/EncodeNumber.cpp)

#---- FeatureLabelTest
create_test_project(NAME FeatureLabelTest SOURCES src/FeatureLabelTest.cpp)

#---- FileScannerWriter
create_test_project(NAME FileScannerWriter SOURCES src/FileScannerWriter.cpp)

#---- GeoCoordParse
create_test_project(NAME GeoCoordParse SOURCES src/GeoCoordParse.cpp)

#---- NumberSet
create_test_project(NAME NumberSet SOURCES src/NumberSet.cpp)

#---- ScanConversion
create_test_project(NAME ScanConversion SOURCES src/ScanConversion.cpp)

#---- StringUtils
create_test_project(NAME StringUtils SOURCES src/StringUtils.cpp)

#---- TilingTest
create_test_project(NAME TilingTest SOURCES src/TilingTest.cpp)

#---- TimeParse
create_test_project(NAME TimeParse SOURCES src/TimeParse.cpp)

#---- WStringStringConversion
create_test_project(NAME WStringStringConversion SOURCES src/WStringStringConversion.cpp)

#---- TransPolygon
create_test_project(NAME TransPolygon SOURCES src/TransPolygon.cpp)

#---- GeoBox
create_test_project(NAME GeoBox SOURCES src/GeoBox.cpp)

#---- OSTAndOSSCheck
if(${OSMSCOUT_BUILD_MAP} AND TARGET OSMScout::Map)
	create_test_project(NAME OSTAndOSSCheck SOURCES src/OSTAndOSSCheck.cpp TARGET OSMScout::Map SKIPTEST)

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
if(${OSMSCOUT_BUILD_MAP} AND TARGET OSMScout::Map)
	create_test_project(NAME LabelPathTest SOURCES src/LabelPathTest.cpp TARGET OSMScout::Map)
else()
	message("Skip LabelPathTest, libosmscout-map is missing.")
endif()

#---- Base64
create_test_project(NAME Base64 SOURCES src/Base64.cpp)

#---- CoordBufferTest
create_test_project(NAME CoordBufferTest SOURCES src/CoordBufferTest.cpp)

#---- ClientQtThreading
if(${OSMSCOUT_BUILD_CLIENT_QT} AND TARGET OSMScout::MapQt AND TARGET OSMScout::ClientQt)
	set(src_files src/ClientQtThreading.cpp)
	qt5_wrap_cpp(src_files include/ClientQtThreading.h)
	create_test_project(NAME ClientQtThreading SOURCES ${src_files} TARGET OSMScout::Map OSMScout::MapQt OSMScout::ClientQt Qt5::Widgets SKIPTEST)
else()
	message("Skip ClientQtThreading test, libosmscout-client-qt is missing.")
endif()

#---- QtFileDownloader
if(${OSMSCOUT_BUILD_CLIENT_QT} AND TARGET OSMScout::MapQt AND TARGET OSMScout::ClientQt)
	set(src_files src/QtFileDownloader.cpp)
	qt5_wrap_cpp(src_files include/DownloaderTest.h)
	create_test_project(NAME QtFileDownloader SOURCES ${src_files} TARGET OSMScout::Map OSMScout::MapQt OSMScout::ClientQt Qt5::Widgets SKIPTEST)
else()
	message("Skip QtFileDownloader test, libosmscout-client-qt is missing.")
endif()
