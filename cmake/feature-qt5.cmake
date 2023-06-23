if(${QT_VERSION_MINOR} STREQUAL "")
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Qml Quick Svg Positioning Multimedia LinguistTools QUIET)
else()
    find_package(Qt5 ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR} REQUIRED COMPONENTS Core Gui Widgets Qml Quick Svg Positioning Multimedia LinguistTools QUIET)
endif()

if(OSMSCOUT_BUILD_MAP_QT)
    if(NOT ${OSMSCOUT_BUILD_MAP})
        message(FATAL_ERROR "OSMSCOUT_BUILD_MAP_QT requires OSMSCOUT_BUILD_MAP")
    endif()

    if(NOT Qt5Gui_FOUND)
        message(FATAL_ERROR "OSMSCOUT_BUILD_MAP_QT requires Qt5Gui")
    endif()

    if(NOT Qt5Svg_FOUND)
        message(FATAL_ERROR "OSMSCOUT_BUILD_MAP_QT requires Qt5Svg")
    endif()
endif()

if(OSMSCOUT_BUILD_CLIENT_QT)
    if(NOT ${OSMSCOUT_BUILD_MAP_QT})
        message(FATAL_ERROR "OSMSCOUT_BUILD_CLIENT_QT requires Qt5Core")
    endif()

    if(NOT ${Qt5Core_FOUND})
        message(FATAL_ERROR "OSMSCOUT_BUILD_CLIENT_QT requires Qt5Core")
    endif()

    if(NOT ${Qt5Gui_FOUND})
        message(FATAL_ERROR "OSMSCOUT_BUILD_CLIENT_QT requires Qt5Gui")
    endif()

    if(NOT ${Qt5Quick_FOUND})
        message(FATAL_ERROR "OSMSCOUT_BUILD_CLIENT_QT requires Qt5Quick")
    endif()

    if(NOT ${Qt5Multimedia_FOUND})
        message(FATAL_ERROR "OSMSCOUT_BUILD_CLIENT_QT requires Qt5Multimedia")
    endif()

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
if(OSMSCOUT_BUILD_TOOL_OSMSCOUT2)
    if(NOT ${OSMSCOUT_BUILD_CLIENT_QT})
        message(FATAL_ERROR "OSMScout2 requires OSMSCOUT_BUILD_CLIENT_QT")
    endif()

    if(NOT ${OSMSCOUT_BUILD_GPX})
        message(FATAL_ERROR "OSMScout2 requires lib gpx")
    endif()

    if(NOT Qt5Core_FOUND)
        message(FATAL "OSMScout2 requires Qt5Core")
    endif()

    if(NOT Qt5Gui_FOUND)
        message(FATAL_ERROR "OSMScout2 requires Qt5Gui")
    endif()

    if(NOT Qt5Widgets_FOUND)
        message(FATAL_ERROR "OSMScout2 requires Qt5Widgets")
    endif()

    if(NOT Qt5Qml_FOUND)
        message(FATAL_ERROR "OSMScout2 requires Qt5Qml")
    endif()

    if(NOT Qt5Quick_FOUND)
        message(FATAL_ERROR "OSMScout2 requires Qt5Quick")
    endif()

    add_subdirectory(OSMScout2)
endif()

# StyleEditor
if(OSMSCOUT_BUILD_TOOL_STYLEEDITOR)
    if(NOT ${OSMSCOUT_BUILD_CLIENT_QT})
        message(FATAL_ERROR "StyleEditor requires OSMSCOUT_BUILD_CLIENT_QT")
    endif()

    if(NOT Qt5Core_FOUND)
        message(FATAL_ERROR "StyleEditor requires Qt5Core")
    endif()

    if(NOT Qt5Gui_FOUND)
        message(FATAL_ERROR "StyleEditor requires Qt5Gui")
    endif()

    if(NOT Qt5Widgets_FOUND)
        message(FATAL_ERROR "StyleEditor requires Qt5Widgets")
    endif()

    if(NOT Qt5Qml_FOUND)
        message(FATAL_ERROR "StyleEditor requires Qt5Qml")
    endif()

    if(NOT Qt5Quick_FOUND)
        message(FATAL_ERROR "StyleEditor requires Qt5Quick")
    endif()

    add_subdirectory(StyleEditor)
endif()
