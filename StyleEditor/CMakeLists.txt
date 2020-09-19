set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(HEADER_FILES
	src/FileIO.h
	src/Highlighter.h
	src/MainWindow.h
	src/SettingsDialog.h
	src/StyleAnalyser.h
	)

set(SOURCE_FILES
	src/FileIO.cpp
	src/Highlighter.cpp
	src/MainWindow.cpp
	src/SettingsDialog.cpp
	src/StyleEditor.cpp
	src/StyleAnalyser.cpp

	# qml files in CMake sources make it visible in QtCreator
	qml/AboutDialog.qml
	qml/main.qml
	qml/settings.qml
	qml/SearchGeocodeDialog.qml
	qml/TextEditor.qml
	qml/custom/LocationEdit.qml
	qml/custom/MapDialog.qml
	qml/custom/ScrollIndicator.qml
	qml/custom/LineEdit.qml
	qml/custom/DialogActionButton.qml
	qml/custom/MapButton.qml
	qml/SearchLocationDialog.qml
	qml/MapControl.qml
	)

set(RESOURCE_FILES
    StyleEditor.qrc
)

add_executable(StyleEditor ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})

target_link_libraries(StyleEditor
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
	target_link_libraries(StyleEditor Qt5::Svg)
endif()

if(MARISA_FOUND)
    target_include_directories(StyleEditor PRIVATE ${MARISA_INCLUDE_DIRS})
endif()

# enable QML debuger
if (QT_QML_DEBUG)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_QML_DEBUG")
endif()

set_target_properties(StyleEditor PROPERTIES VERSION ${OSMSCOUT_LIBRARY_VERSION} SOVERSION ${OSMSCOUT_LIBRARY_VERSION} FOLDER "Tools")

if(WIN32)
	create_win32_tool_resource(StyleEditor)
endif()
if(MSVC)
	  visual_studio_qt_helper("StyleEditor")
endif()
if(OSMSCOUT_INSTALL_QT_DLL AND TARGET Qt5::windeployqt)
	add_custom_command(TARGET StyleEditor
        POST_BUILD
        COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
        COMMAND Qt5::windeployqt --dir "${CMAKE_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:OSMScout2>/$<TARGET_FILE_NAME:OSMScout2>"
    )
endif()

install(TARGETS StyleEditor
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
