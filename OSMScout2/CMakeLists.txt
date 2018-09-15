if(NOT ${OSMSCOUT_BUILD_MAP_QT})
	message(SEND_ERROR "The Qt map drawing backend is required for OSMScout2 application")
endif()
if(NOT ${OSMSCOUT_BUILD_CLIENT_QT})
	message(SEND_ERROR "The Qt client lib is required for OSMScout2 application")
endif()
if(NOT ${OSMSCOUT_BUILD_GPX})
    message(SEND_ERROR "The Gpx lib is required for OSMScout2 application")
endif()
if(NOT Qt5Core_FOUND)
	message(SEND_ERROR "Qt5Core is required for OSMScout2 application")
endif()
if(NOT Qt5Gui_FOUND)
	message(SEND_ERROR "Qt5Gui is required for OSMScout2 application")
endif()
if(NOT Qt5Widgets_FOUND)
	message(SEND_ERROR "Qt5Widgets is required for OSMScout2 application")
endif()
if(NOT Qt5Qml_FOUND)
	message(SEND_ERROR "Qt5Qml is required for OSMScout2 application")
endif()
if(NOT Qt5Quick_FOUND)
	message(SEND_ERROR "Qt5Quick is required for OSMScout2 application")
endif()

find_package(Qt5LinguistTools)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

string(TIMESTAMP OSMSCOUT_VERSION_STRING "%Y-%m-%d")
set(OSMSCOUT_USER_AGENT "OSMScout demo app %1")

set(HEADER_FILES
    src/Theme.h
    src/AppSettings.h
	src/PositionSimulator.h
	)

set(SOURCE_FILES
    src/Theme.cpp
    src/AppSettings.cpp
    src/OSMScout.cpp
	src/PositionSimulator.cpp

    # qml files in CMake sources make it visible in QtCreator
    qml/AboutDialog.qml
    qml/main.qml
    qml/SearchDialog.qml
    qml/MapDownloadDialog.qml
	qml/NavigationSimulation.qml
    qml/custom/MapDialog.qml
    qml/custom/ScrollIndicator.qml
    qml/custom/LineEdit.qml
    qml/custom/LocationSearch.qml
    qml/custom/DialogActionButton.qml
    qml/custom/MapButton.qml
	qml/custom/RoutingStep.qml
	qml/custom/RouteStepIcon.qml
	)

# XML files with translated phrases.
# You can add new language translation just by adding new entry here, and run build.
# Newly created *.ts file may be edited by linguist tool.
# Please don't forget to add new *.ts file to Meson-based build.
set(TRANSLATION_TS_FILES
	translations/en.ts
	translations/cs.ts
	)

# source files that may contains translatable phrases (see tr, qsTr, QT_TRANSLATE_NOOP functions)
set(TRANSLATION_SOURCE_FILES
	${SOURCE_FILES}
	../libosmscout-client-qt/src/osmscout/RouteDescriptionBuilder.cpp
	)

# See http://doc.qt.io/qt-5/cmake-manual.html
#
# qt5_create_translation macro process translations in two steps:
#
#  - collect available phrases inside TRANSLATION_SOURCE_FILES
#    with lupdate tool and put them into *.ts files if some is missing there.
#
#  - create binary translation files (*.qm) from translated phrases available
#    in *.ts files with lrelease tool
qt5_create_translation(TRANSLATION_QM_FILES
		${TRANSLATION_SOURCE_FILES}
		${TRANSLATION_TS_FILES})

qt5_add_resources(RESOURCE_FILES res.qrc)

set_property(SOURCE qrc_res.cpp PROPERTY SKIP_AUTOMOC ON)

add_executable(OSMScout2 ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})

target_sources(OSMScout2 PRIVATE ${TRANSLATION_TS_FILES} ${TRANSLATION_QM_FILES})

target_link_libraries(OSMScout2
	OSMScout
	OSMScoutMap
	OSMScoutGPX
	Qt5::Core
	Qt5::Gui
	Qt5::Widgets
	Qt5::Qml
	Qt5::Quick
	OSMScoutMapQt
	OSMScoutClientQt
	OSMScoutGPX
)

if (Qt5Svg_FOUND)
	target_link_libraries(OSMScout2 Qt5::Svg)
endif()

if (Qt5Positioning_FOUND)
	target_link_libraries(OSMScout2 Qt5::Positioning)
endif()

# enable QML debuger
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_QML_DEBUG")
set_property(TARGET OSMScout2 PROPERTY CXX_STANDARD 11)

install(TARGETS OSMScout2 RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

# install QM files to correct location - QStandardPaths::AppDataLocation / "translations"
if(APPLE)
	install(FILES ${TRANSLATION_QM_FILES}
			DESTINATION "/Library/Application Support/libosmscout/OSMScout2/translations")
elseif(WIN32)
#	install(FILES ${TRANSLATION_QM_FILES}
#			DESTINATION C:/ProgramData/libosmscout/OSMScout2/translations)
else() # Linux
	install(FILES ${TRANSLATION_QM_FILES}
			DESTINATION share/libosmscout/OSMScout2/translations)
endif()
