#---- Coverage
osmscout_demo_project(NAME Coverage SOURCES src/Coverage.cpp TARGET OSMScout::OSMScout)

#---- DumpOSS
if(${OSMSCOUT_BUILD_MAP})
  osmscout_demo_project(NAME DumpOSS SOURCES src/DumpOSS.cpp TARGET OSMScout::OSMScout OSMScout::Map)
else()
  message("Skip DumpOSS demo libosmscout-map, is missing.")
endif()

#---- LocationDescription
osmscout_demo_project(NAME LocationDescription SOURCES src/LocationDescription.cpp TARGET OSMScout::OSMScout)

#---- LocationLookup
osmscout_demo_project(NAME LocationLookup SOURCES src/LocationLookup.cpp TARGET OSMScout::OSMScout)

#---- LocationLookupForm
osmscout_demo_project(NAME LocationLookupForm SOURCES src/LocationLookupForm.cpp TARGET OSMScout::OSMScout)

#---- POILookupForm
osmscout_demo_project(NAME POILookupForm SOURCES src/POILookupForm.cpp TARGET OSMScout::OSMScout)

#---- LookupPOI
osmscout_demo_project(NAME LookupPOI SOURCES src/LookupPOI.cpp TARGET OSMScout::OSMScout)

#---- LookupText
if(MARISA_FOUND)
  osmscout_demo_project(NAME LookupText SOURCES src/LookupText.cpp TARGET OSMScout::OSMScout INCLUDES ${MARISA_INCLUDE_DIRS})
else()
  message("Skip LookupText demo, marisa dependency is missing.")
endif()

#---- ResourceConsumption
if(${OSMSCOUT_BUILD_MAP})
	osmscout_demo_project(NAME ResourceConsumption SOURCES src/ResourceConsumption.cpp TARGET OSMScout::OSMScout OSMScout::Map)
else()
	message("Skip ResourceConsumption demo, libosmscout-map is missing.")
endif()

#---- ReverseLocationLookup
osmscout_demo_project(NAME ReverseLocationLookup SOURCES src/ReverseLocationLookup.cpp TARGET OSMScout::OSMScout)

#---- Routing
osmscout_demo_project(NAME Routing SOURCES src/Routing.cpp TARGET OSMScout::OSMScout)

#---- RoutingAnimation
if(${OSMSCOUT_BUILD_MAP_QT})
	osmscout_demo_project(NAME RoutingAnimation SOURCES src/RoutingAnimation.cpp TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapQt Qt::Widgets)
	if(MSVC)
		  visual_studio_qt_helper("RoutingAnimation")
	endif()
	if(OSMSCOUT_INSTALL_QT_DLL AND TARGET Qt::windeployqt)
		add_custom_command(TARGET RoutingAnimation
			POST_BUILD
			COMMAND set PATH=%PATH%$<SEMICOLON>${qt_install_prefix}/bin
			COMMAND Qt::windeployqt --dir "${CMAKE_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:RoutingAnimation>/$<TARGET_FILE_NAME:RoutingAnimation>"
		)
	endif()
endif()

#---- Tiler & DrawMapAgg
if(${OSMSCOUT_BUILD_MAP_AGG})
	#---- Tiler
	osmscout_demo_project(NAME Tiler SOURCES src/Tiler.cpp TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapAGG)
    if(LIBAGGFT2_LIBRARIES)
        target_link_libraries(Tiler ${LIBAGGFT2_LIBRARIES})
    endif()

	#---- DrawMapAgg
	osmscout_demo_project(NAME DrawMapAgg SOURCES src/DrawMapAgg.cpp TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapAGG)
    if(LIBAGGFT2_LIBRARIES)
        target_link_libraries(DrawMapAgg ${LIBAGGFT2_LIBRARIES})
    endif()
else()
	message("Skip Tiler and DrawMapAgg demos, libosmscout-map-agg is missing.")
endif()

#---- DrawMapCairo
if(${OSMSCOUT_BUILD_MAP_CAIRO})
	osmscout_demo_project(NAME DrawMapCairo SOURCES src/DrawMapCairo.cpp TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapCairo INCLUDES ${CAIRO_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS}/../ ${CAIRO_INCLUDE_DIRS}/cairo)
else()
	message("Skip DrawMapCairo demo, libosmscout-map-cairo is missing.")
endif()

#---- DrawMapQt & ResourceConsumptionQt
if(${OSMSCOUT_BUILD_MAP_QT})
	#---- DrawMapQt
    osmscout_demo_project(NAME DrawMapQt SOURCES src/DrawMapQt.cpp TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapQt Qt::Widgets)
    target_compile_definitions(DrawMapQt PRIVATE ${QtWidgets_DEFINITIONS})
	if(MSVC)
		  visual_studio_qt_helper("DrawMapQt")
	endif()
	if(OSMSCOUT_INSTALL_QT_DLL AND TARGET Qt::windeployqt)
		add_custom_command(TARGET DrawMapQt
			POST_BUILD
			COMMAND set PATH=%PATH%$<SEMICOLON>${qt_install_prefix}/bin
			COMMAND Qt::windeployqt --dir "${CMAKE_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:DrawMapQt>/$<TARGET_FILE_NAME:DrawMapQt>"
		)
	endif()

	#---- ResourceConsumptionQt
    osmscout_demo_project(NAME ResourceConsumptionQt SOURCES src/ResourceConsumptionQt.cpp TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapQt Qt::Widgets)
    target_compile_definitions(ResourceConsumptionQt PRIVATE ${QtWidgets_DEFINITIONS})
	if(MSVC)
		  visual_studio_qt_helper("ResourceConsumptionQt")
	endif()
	if(OSMSCOUT_INSTALL_QT_DLL AND TARGET Qt::windeployqt)
		add_custom_command(TARGET ResourceConsumptionQt
			POST_BUILD
			COMMAND set PATH=%PATH%$<SEMICOLON>${qt_install_prefix}/bin
			COMMAND Qt::windeployqt --dir "${CMAKE_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:ResourceConsumptionQt>/$<TARGET_FILE_NAME:ResourceConsumptionQt>"
		)
	endif()
else()
	message("Skip ResourceConsumptionQt and DrawMapQt demos, libosmscout-map-qt is missing.")
endif()

#---- ElevationProfileChart
if(${OSMSCOUT_BUILD_CLIENT_QT})
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)

	qt_add_resources(RESOURCE_FILES ../OSMScout2/res.qrc demo.qrc)

	set_property(SOURCE qrc_res.cpp PROPERTY SKIP_AUTOMOC ON)
	set_property(SOURCE qrc_demo.cpp PROPERTY SKIP_AUTOMOC ON)

	#---- ElevationProfileChart
	osmscout_demo_project(NAME ElevationProfileChart
			SOURCES
				src/ElevationProfileChart.cpp
				src/QtDemoApp.cpp
				${RESOURCE_FILES}

				# qml files in CMake sources make it visible in QtCreator
				qml/ElevationProfileChart.qml

			TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapQt OSMScout::ClientQt Qt::Widgets)
	target_compile_definitions(ElevationProfileChart PRIVATE ${QtWidgets_DEFINITIONS})
	set_target_properties(ElevationProfileChart PROPERTIES
		UNITY_BUILD OFF) # generated code for qt resources use static variables with the same name
	if(MSVC)
		visual_studio_qt_helper("ElevationProfileChart")
	endif()
	if(OSMSCOUT_INSTALL_QT_DLL AND TARGET Qt::windeployqt)
		add_custom_command(TARGET ElevationProfileChart
				POST_BUILD
				COMMAND set PATH=%PATH%$<SEMICOLON>${qt_install_prefix}/bin
				COMMAND Qt::windeployqt --dir "${CMAKE_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:ElevationProfileChart>/$<TARGET_FILE_NAME:ElevationProfileChart>"
				)
	endif()

	if (${OSMSCOUT_BUILD_GPX})

		#---- NavigationSimulation
		osmscout_demo_project(NAME NavigationSimulation
			SOURCES
			src/NavigationSimulation.cpp
			include/PositionSimulator.h src/PositionSimulator.cpp
			../OSMScout2/src/Theme.h ../OSMScout2/src/Theme.cpp
			src/QtDemoApp.cpp
			${RESOURCE_FILES}

			# qml files in CMake sources make it visible in QtCreator
			qml/NavigationSimulation.qml

			INCLUDES ../OSMScout2/src/
			TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapQt OSMScout::ClientQt OSMScout::GPX Qt::Widgets)
		target_compile_definitions(NavigationSimulation PRIVATE ${QtWidgets_DEFINITIONS})
		set_target_properties(NavigationSimulation PROPERTIES
			UNITY_BUILD OFF) # generated code for qt resources use static variables with the same name
		if(MSVC)
			visual_studio_qt_helper("NavigationSimulation")
		endif()
		if(OSMSCOUT_INSTALL_QT_DLL AND TARGET Qt::windeployqt)
			add_custom_command(TARGET NavigationSimulation
				POST_BUILD
				COMMAND set PATH=%PATH%$<SEMICOLON>${qt_install_prefix}/bin
				COMMAND Qt::windeployqt --dir "${CMAKE_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:NavigationSimulation>/$<TARGET_FILE_NAME:NavigationSimulation>"
				)
		endif()
	else()
		message("Skip NavigationSimulation demo, libosmscout-gpx is missing.")
	endif()

	#---- RoutingParameters
	osmscout_demo_project(NAME RoutingParameters
		SOURCES
			../OSMScout2/src/Theme.h ../OSMScout2/src/Theme.cpp
			src/RoutingParameters.cpp
			src/QtDemoApp.cpp
			${RESOURCE_FILES}

			# qml files in CMake sources make it visible in QtCreator
			qml/RoutingParameters.qml
			qml/PlaceInput.qml

		INCLUDES ../OSMScout2/src/
		TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapQt OSMScout::ClientQt Qt::Widgets)
	target_compile_definitions(RoutingParameters PRIVATE ${QtWidgets_DEFINITIONS})
	set_target_properties(RoutingParameters PROPERTIES
		UNITY_BUILD OFF) # generated code for qt resources use static variables with the same name
	if(MSVC)
		visual_studio_qt_helper("RoutingParameters")
	endif()
	if(OSMSCOUT_INSTALL_QT_DLL AND TARGET Qt::windeployqt)
		add_custom_command(TARGET RoutingParameters
			POST_BUILD
			COMMAND set PATH=%PATH%$<SEMICOLON>${qt_install_prefix}/bin
			COMMAND Qt::windeployqt --dir "${CMAKE_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:RoutingParameters>/$<TARGET_FILE_NAME:RoutingParameters>"
			)
	endif()

	#---- QtWidgetsDemoApp
	osmscout_demo_project(NAME QtWidgetsDemoApp SOURCES src/QtWidgetsDemoApp.cpp TARGET Qt::Widgets OSMScout::ClientQt)
else()
	message("Skip ElevationProfileChart, NavigationSimulation and RoutingParameters QtWidgetsDemoApp demo, libosmscout-client-qt is missing.")
endif()

#---- DrawMapSVG
if(${OSMSCOUT_BUILD_MAP_SVG})
    osmscout_demo_project(NAME DrawMapSVG SOURCES src/DrawMapSVG.cpp TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapSVG)
else()
	message("Skip DrawMapSVG demo, libosmscout-map-svg is missing.")
endif()

#---- DrawMapDirectX
if(${OSMSCOUT_BUILD_MAP_DIRECTX})
    osmscout_demo_project(NAME DrawMapDirectX SOURCES src/DrawMapDirectX.cpp TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapDirectX ${Direct2D_LIBRARIES} "Shell32.lib")
	if(MSVC)
		set_target_properties(DrawMapDirectX PROPERTIES LINK_FLAGS /SUBSYSTEM:WINDOWS)
	endif()
else()
	message("Skip DrawMapDirectX demo, libosmscout-map-directx is missing.")
endif()

#---- DrawMapOpenGL
if(${OSMSCOUT_BUILD_MAP_OPENGL} AND ${GLFW_FOUND})
    osmscout_demo_project(NAME DrawMapOpenGL SOURCES src/DrawMapOpenGL.cpp TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapOpenGL ${GLFW_LIBRARIES})
    target_include_directories(DrawMapOpenGL PRIVATE ${GLFW_INCLUDE_DIR})
else()
	message("Skip DrawMapOpenGL demo, libosmscout-map-opengl or glfw is missing.")
endif()

#---- DrawMapOSX
if(APPLE AND NOT IOS)
	if(${OSMSCOUT_BUILD_MAP_IOSX})
		osmscout_demo_project(NAME DrawMapOSX SOURCES src/DrawMapOSX.mm TARGET OSMScout::OSMScout OSMScout::Map OSMScoutMapIOSX INCLUDES ${CMAKE_SOURCE_DIR}/libosmscout-map-iOSX/include ${CMAKE_BINARY_DIR}/libosmscout-map-iOSX/include)
		set_target_properties(DrawMapOSX PROPERTIES
				UNITY_BUILD OFF)
	else()
		message("Skip DrawMapOSX demo, libosmscout-map-iosx is missing.")
	endif()
endif()

#---- GpxPipe & ElevationProfile
if(${LIBXML2_FOUND} AND ${OSMSCOUT_BUILD_GPX})
    #---- GpxPipe
    osmscout_demo_project(NAME GpxPipe SOURCES src/GpxPipe.cpp TARGET OSMScout::OSMScout OSMScout::GPX)

    #---- ElevationProfile
    osmscout_demo_project(NAME ElevationProfile SOURCES src/ElevationProfile.cpp TARGET OSMScout::OSMScout OSMScout::GPX)
else()
    message("Skip GpxPipe and ElevationProfile demos, libxml is missing.")
endif()

#---- Navigation
osmscout_demo_project(NAME Navigation SOURCES src/Navigation.cpp TARGET OSMScout::OSMScout)

#---- NavigationSimulator
osmscout_demo_project(NAME NavigationSimulator SOURCES src/NavigationSimulator.cpp TARGET OSMScout::OSMScout OSMScout::Map)

#---- DrawMapGDI
if(${OSMSCOUT_BUILD_MAP_GDI})
    osmscout_demo_project(NAME DrawMapGDI SOURCES src/DrawMapGDI.cpp TARGET OSMScout::OSMScout OSMScout::Map OSMScout::MapGDI)
    target_compile_definitions(DrawMapGDI PRIVATE _UNICODE UNICODE)
	if(MSVC)
		set_target_properties(DrawMapGDI PROPERTIES LINK_FLAGS /SUBSYSTEM:WINDOWS)
	endif()
else()
	message("Skip DrawMapGDI demo, libosmscout-map-gdi is missing.")
endif()

#---- Srtm
osmscout_demo_project(NAME Srtm SOURCES src/Srtm.cpp TARGET OSMScout::OSMScout)
