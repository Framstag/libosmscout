set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(HEADER_FILES
          src/DocumentHandler.h
          src/Highlighter.h
          src/MainWindow.h
          src/SettingsDialog.h
          src/StyleAnalyser.h
          )

set(SOURCE_FILES
          src/DocumentHandler.cpp
          src/Highlighter.cpp
          src/MainWindow.cpp
          src/SettingsDialog.cpp
          src/StyleEditor.cpp
          src/StyleAnalyser.cpp
          )

if(QT_VERSION_MAJOR EQUAL 5)
  set(RESOURCE_FILES StyleEditor_515.qrc)
  list(APPEND SOURCE_FILES
      # qml files in CMake sources make it visible in QtCreator
      qml_515/AboutDialog.qml
      qml_515/main.qml
      qml_515/SearchGeocodeDialog.qml
      qml_515/TextEditor.qml
      qml_515/Units.qml
      qml_515/custom/LocationEdit.qml
      qml_515/custom/ScrollIndicator.qml
      qml_515/custom/LineEdit.qml
      qml_515/custom/DialogBase.qml
      qml_515/custom/DialogAction.qml
      qml_515/custom/MapButton.qml
      qml_515/SearchLocationDialog.qml
      qml_515/MapControl.qml
      )
elseif(QT_VERSION_MAJOR EQUAL 6)
  set(RESOURCE_FILES StyleEditor_640.qrc)
  list(APPEND SOURCE_FILES
      # qml files in CMake sources make it visible in QtCreator
      qml_640/AboutDialog.qml
      qml_640/main.qml
      qml_640/SearchGeocodeDialog.qml
      qml_640/TextEditor.qml
      qml_640/Units.qml
      qml_640/custom/LocationEdit.qml
      qml_640/custom/ScrollIndicator.qml
      qml_640/custom/LineEdit.qml
      qml_640/custom/DialogBase.qml
      qml_640/custom/DialogAction.qml
      qml_640/custom/MapButton.qml
      qml_640/SearchLocationDialog.qml
      qml_640/MapControl.qml
      )
else()
  message(FATAL_ERROR "Missing Qt dependencies")
endif()

add_executable(StyleEditor ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})

target_include_directories(StyleEditor PRIVATE "src")

target_link_libraries(StyleEditor
          OSMScout::OSMScout
          OSMScout::Map
          OSMScout::MapQt
          OSMScout::ClientQt
          Qt::Core
          Qt::Gui
          Qt::Widgets
          Qt::Qml
          Qt::Quick
          Qt::Svg
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(StyleEditor Qt::Core5Compat)
endif()

if(MARISA_FOUND)
    target_include_directories(StyleEditor PRIVATE ${MARISA_INCLUDE_DIRS})
endif()

# enable QML debuger
if (QT_QML_DEBUG)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_QML_DEBUG")
endif()

set_target_properties(StyleEditor
                    PROPERTIES
                    VERSION ${OSMSCOUT_LIBRARY_VERSION}
                    SOVERSION ${OSMSCOUT_LIBRARY_VERSION}
                    FOLDER "Tools")

if(WIN32)
          create_win32_tool_resource(StyleEditor)
endif()
if(MSVC)
            visual_studio_qt_helper("StyleEditor")
endif()
if(OSMSCOUT_INSTALL_QT_DLL AND TARGET Qt::windeployqt)
          add_custom_command(TARGET StyleEditor
        POST_BUILD
        COMMAND set PATH=%PATH%$<SEMICOLON>${qt_install_prefix}/bin
        COMMAND Qt::windeployqt --dir "${CMAKE_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:OSMScout2>/$<TARGET_FILE_NAME:OSMScout2>"
    )
endif()

install(TARGETS StyleEditor
        RUNTIME DESTINATION bin
                    BUNDLE DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
