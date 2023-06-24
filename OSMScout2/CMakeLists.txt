find_package(Qt5LinguistTools)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

string(TIMESTAMP OSMSCOUT_VERSION_STRING "%Y-%m-%d")
set(OSMSCOUT_USER_AGENT "OSMScout demo app %1")

set(HEADER_FILES
    src/Theme.h
    src/AppSettings.h
	)

set(SOURCE_FILES
    src/Theme.cpp
    src/AppSettings.cpp
    src/OSMScout.cpp

    # qml files in CMake sources make it visible in QtCreator
    qml/AboutDialog.qml
    qml/main.qml
    qml/SearchDialog.qml
    qml/MapDownloadDialog.qml
    qml/VoiceDownloadDialog.qml
    qml/custom/MapDialog.qml
    qml/custom/ScrollIndicator.qml
    qml/custom/LineEdit.qml
    qml/custom/LocationSearch.qml
    qml/custom/DialogActionButton.qml
    qml/custom/MapButton.qml
	qml/custom/RoutingStep.qml
	qml/custom/RouteStepIcon.qml
	qml/custom/LaneTurnIcon.qml
	qml/custom/LaneTurns.qml
	qml/custom/Utils.js
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
	../libosmscout-client-qt/src/osmscoutclientqt/RouteDescriptionBuilder.cpp
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

target_include_directories(OSMScout2 PRIVATE "src")

target_sources(OSMScout2 PRIVATE ${TRANSLATION_TS_FILES} ${TRANSLATION_QM_FILES})

target_link_libraries(OSMScout2
	OSMScout::OSMScout
	OSMScout::Map
	OSMScout::MapQt
	OSMScout::ClientQt
	Qt5::Core
	Qt5::Gui
	Qt5::Widgets
	Qt5::Qml
	Qt5::Quick
)

if (Qt5Svg_FOUND)
	target_link_libraries(OSMScout2 Qt5::Svg)
endif()

if (Qt5Positioning_FOUND)
	target_link_libraries(OSMScout2 Qt5::Positioning)
endif()

if(MARISA_FOUND)
    target_include_directories(OSMScout2 PRIVATE ${MARISA_INCLUDE_DIRS})
endif()

# enable QML debuger
if (QT_QML_DEBUG)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_QML_DEBUG")
endif()

set_target_properties(OSMScout2
		PROPERTIES
		VERSION ${OSMSCOUT_LIBRARY_VERSION}
		SOVERSION ${OSMSCOUT_LIBRARY_VERSION}
		FOLDER "Tools")

if(WIN32)
	create_win32_tool_resource(OSMScout2)
endif()
if(MSVC)
	  visual_studio_qt_helper("OSMScout2")
endif()
if(OSMSCOUT_INSTALL_QT_DLL AND TARGET Qt5::windeployqt)
	add_custom_command(TARGET OSMScout2
        POST_BUILD
        COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
        COMMAND Qt5::windeployqt --dir "${CMAKE_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:OSMScout2>/$<TARGET_FILE_NAME:OSMScout2>"
    )
endif()

install(TARGETS OSMScout2
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)

# install QM files to correct location - QStandardPaths::AppDataLocation / "translations"
if(APPLE)
	install(FILES ${TRANSLATION_QM_FILES}
			DESTINATION "/Library/Application Support/osmscout/OSMScout2/translations")
elseif(WIN32)
#	install(FILES ${TRANSLATION_QM_FILES}
#			DESTINATION C:/ProgramData/osmscout/OSMScout2/translations)
else() # Linux
	install(FILES ${TRANSLATION_QM_FILES}
			DESTINATION share/osmscout/OSMScout2/translations)
endif()
