set(OSMSCOUT_BUILD_MAP_QT_FOR_QT5_ENABLED OFF)
set(OSMSCOUT_BUILD_CLIENT_QT_FOR_QT5_ENABLED OFF)
set(OSMSCOUT_BUILD_TOOL_OSMSCOUT2_FOR_QT5_ENABLED OFF)
set(OSMSCOUT_BUILD_TOOL_STYLEEDITOR_FOR_QT5_ENABLED OFF)

if(NOT ${QT_MINOR_VERSION} OR ${QT_MINOR_VERSION} STREQUAL "")
    find_package(Qt5
                 REQUIRE
                 QUIET
                 COMPONENTS Core Gui Widgets Qml Quick Svg Positioning Multimedia LinguistTools
                 PATHS "/usr/local/Cellar/qt@5/5.15.10/")
else()
    find_package(Qt5
                 ${QT_MAJOR_VERSION}.${QT_MINOR_VERSION}
                 REQUIRE
                 QUIET
                 COMPONENTS Core Gui Widgets Qml Quick Svg Positioning Multimedia LinguistTools
                 PATHS "/usr/local/Cellar/qt@5/5.15.10/")
endif()

message(STATUS "Qt5Core_FOUND: ", ${Qt5Core_FOUND})
message(STATUS "Qt5Gui_FOUND: ", ${Qt5Gui_FOUND})
message(STATUS "Qt5Widgets_FOUND: ", ${Qt5Widgets_FOUND})
message(STATUS "Qt5Qml_FOUND: ", ${Qt5Qml_FOUND})
message(STATUS "Qt5Quick_FOUND: ", ${Qt5Quick_FOUND})
message(STATUS "Qt5Svg_FOUND: ", ${Qt5Svg_FOUND})
message(STATUS "Qt5Positioning_FOUND: ", ${Qt5Positioning_FOUND})
message(STATUS "Qt5Multimedia_FOUND: ", ${Qt5Multimedia_FOUND})
message(STATUS "Qt5LinguistTools_FOUND: ", ${Qt5LinguistTools_FOUND})

set(HAVE_LIB_QT5_GUI ${Qt5Gui_FOUND})
set(HAVE_LIB_QT5_WIDGETS ${Qt5Widgets_FOUND})

if(${OSMSCOUT_BUILD_MAP_QT})
    if(NOT ${OSMSCOUT_BUILD_MAP})
        message(VERBOSE "OSMSCOUT_BUILD_MAP_QT requires OSMSCOUT_BUILD_MAP")
    endif()

    if(NOT Qt5Gui_FOUND)
        message(VERBOSE "OSMSCOUT_BUILD_MAP_QT requires Qt5Gui")
    endif()

    if(NOT Qt5Svg_FOUND)
        message(VERBOSE "OSMSCOUT_BUILD_MAP_QT requires Qt5Svg")
    endif()

    add_subdirectory(libosmscout-map-qt)

    set(OSMSCOUT_BUILD_MAP_QT_FOR_QT5_ENABLED ON)
endif()

if(${OSMSCOUT_BUILD_CLIENT_QT})
    if(NOT ${OSMSCOUT_BUILD_MAP_QT})
        message(VERBOSE "OSMSCOUT_BUILD_CLIENT_QT requires Qt5Core")
    endif()

    if(NOT ${Qt5Core_FOUND})
        message(VERBOSE "OSMSCOUT_BUILD_CLIENT_QT requires Qt5Core")
    endif()

    if(NOT ${Qt5Gui_FOUND})
        message(VERBOSE "OSMSCOUT_BUILD_CLIENT_QT requires Qt5Gui")
    endif()

    if(NOT ${Qt5Quick_FOUND})
        message(VERBOSE "OSMSCOUT_BUILD_CLIENT_QT requires Qt5Quick")
    endif()

    if(NOT ${Qt5Multimedia_FOUND})
        message(VERBOSE "OSMSCOUT_BUILD_CLIENT_QT requires Qt5Multimedia")
    endif()

    add_subdirectory(libosmscout-client-qt)

    set(OSMSCOUT_BUILD_CLIENT_QT_FOR_QT5_ENABLED ON)
    
    option(QT_QML_DEBUG "Build with QML debugger support" OFF)
    mark_as_advanced(QT_QML_DEBUG)
endif()

################################################################
# Qt DLLs
################################################################
if(WIN32 AND TARGET Qt5::qmake AND NOT TARGET Qt5::windeployqt)
get_target_property(_qt5_qmake_location Qt5::qmake IMPORTED_LOCATION)
execute_process(
    COMMAND "${_qt5_qmake_location}" -query QT_INSTALL_PREFIX
    RESULT_VARIABLE return_code
    OUTPUT_VARIABLE qt5_install_prefix
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(imported_location "${qt5_install_prefix}/bin/windeployqt.exe")
if(EXISTS ${imported_location})
    add_executable(Qt5::windeployqt IMPORTED libosmscout-client/src/osmscoutclient/Empty.cpp)
    set_target_properties(Qt5::windeployqt PROPERTIES IMPORTED_LOCATION ${imported_location})
endif()
option(OSMSCOUT_INSTALL_QT_DLL "Copies the DLLs from Qt to the installation directory" OFF)
mark_as_advanced(OSMSCOUT_INSTALL_QT_DLL)
endif()
################################################################

# OSMScout2
if(${OSMSCOUT_BUILD_TOOL_OSMSCOUT2})
    if(NOT ${OSMSCOUT_BUILD_CLIENT_QT})
        message(STATUS "OSMScout2 requires OSMSCOUT_BUILD_CLIENT_QT")
    endif()

    if(NOT ${OSMSCOUT_BUILD_GPX})
        message(STATUS "OSMScout2 requires lib gpx")
    endif()

    if(NOT ${Qt5Core_FOUND})
        message(STATUS "OSMScout2 requires Qt5Core")
    endif()

    if(NOT ${Qt5Gui_FOUND})
        message(STATUS "OSMScout2 requires Qt5Gui")
    endif()

    if(NOT ${Qt5Widgets_FOUND})
        message(STATUS "OSMScout2 requires Qt5Widgets")
    endif()

    if(NOT ${Qt5Qml_FOUND})
        message(STATUS "OSMScout2 requires Qt5Qml")
    endif()

    if(NOT ${Qt5Quick_FOUND})
        message(STATUS "OSMScout2 requires Qt5Quick")
    endif()

    add_subdirectory(OSMScout2)

    set(OSMSCOUT_BUILD_TOOL_OSMSCOUT2_FOR_QT5_ENABLED ON)
endif()

# StyleEditor
if(${OSMSCOUT_BUILD_TOOL_STYLEEDITOR})
    if(NOT ${OSMSCOUT_BUILD_CLIENT_QT})
        message(STATUS "StyleEditor requires OSMSCOUT_BUILD_CLIENT_QT")
    endif()

    if(NOT ${Qt5Core_FOUND})
        message(STATUS "StyleEditor requires Qt5Core")
    endif()

    if(NOT ${Qt5Gui_FOUND})
        message(STATUS "StyleEditor requires Qt5Gui")
    endif()

    if(NOT ${Qt5Widgets_FOUND})
        message(STATUS "StyleEditor requires Qt5Widgets")
    endif()

    if(NOT ${Qt5Qml_FOUND})
        message(STATUS "StyleEditor requires Qt5Qml")
    endif()

    if(NOT ${Qt5Quick_FOUND})
        message(STATUS "StyleEditor requires Qt5Quick")
    endif()

    add_subdirectory(StyleEditor)

    set(OSMSCOUT_BUILD_TOOL_STYLEEDITOR_FOR_QT5_ENABLED ON)
endif()
