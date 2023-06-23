if(${QT_VERSION_MINOR} STREQUAL "")
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick Svg Positioning Multimedia LinguistTools QUIET)
else()
    find_package(Qt6 ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR} REQUIRED COMPONENTS Core Gui Widgets Qml Quick Svg Positioning Multimedia LinguistTools QUIET)
endif()

if(OSMSCOUT_BUILD_MAP_QT)
    if(NOT ${OSMSCOUT_BUILD_MAP})
        message(FATAL_ERROR "OSMSCOUT_BUILD_MAP_QT requires OSMSCOUT_BUILD_MAP")
    endif()

    if(NOT Qt6Gui_FOUND)
        message(FATAL_ERROR "OSMSCOUT_BUILD_MAP_QT requires Qt6Gui")
    endif()

    if(NOT Qt6Svg_FOUND)
        message(FATAL_ERROR "OSMSCOUT_BUILD_MAP_QT requires Qt6Svg")
    endif()
endif()

if(OSMSCOUT_BUILD_CLIENT_QT)
    if(NOT ${OSMSCOUT_BUILD_MAP_QT})
        message(FATAL_ERROR "OSMSCOUT_BUILD_CLIENT_QT requires Qt6Core")
    endif()

    if(NOT ${Qt6Core_FOUND})
        message(FATAL_ERROR "OSMSCOUT_BUILD_CLIENT_QT requires Qt6Core")
    endif()

    if(NOT ${Qt6Gui_FOUND})
        message(FATAL_ERROR "OSMSCOUT_BUILD_CLIENT_QT requires Qt6Gui")
    endif()

    if(NOT ${Qt6Quick_FOUND})
        message(FATAL_ERROR "OSMSCOUT_BUILD_CLIENT_QT requires Qt6Quick")
    endif()

    if(NOT ${Qt6Multimedia_FOUND})
        message(FATAL_ERROR "OSMSCOUT_BUILD_CLIENT_QT requires Qt6Multimedia")
    endif()

    option(QT_QML_DEBUG "Build with QML debugger support" OFF)
    mark_as_advanced(QT_QML_DEBUG)
endif()

################################################################
# Qt DLLs
################################################################
if(WIN32 AND TARGET Qt6::qmake AND NOT TARGET Qt6::windeployqt)
get_target_property(_qt6_qmake_location Qt6::qmake IMPORTED_LOCATION)
execute_process(
    COMMAND "${_qt6_qmake_location}" -query QT_INSTALL_PREFIX
    RESULT_VARIABLE return_code
    OUTPUT_VARIABLE qt6_install_prefix
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(imported_location "${qt6_install_prefix}/bin/windeployqt.exe")
if(EXISTS ${imported_location})
    add_executable(Qt6::windeployqt IMPORTED libosmscout-client/src/osmscoutclient/Empty.cpp)
    set_target_properties(Qt6::windeployqt PROPERTIES IMPORTED_LOCATION ${imported_location})
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

    if(NOT Qt6Core_FOUND)
        message(FATAL "OSMScout2 requires Qt6Core")
    endif()

    if(NOT Qt6Gui_FOUND)
        message(FATAL_ERROR "OSMScout2 requires Qt6Gui")
    endif()

    if(NOT Qt6Widgets_FOUND)
        message(FATAL_ERROR "OSMScout2 requires Qt6Widgets")
    endif()

    if(NOT Qt6Qml_FOUND)
        message(FATAL_ERROR "OSMScout2 requires Qt6Qml")
    endif()

    if(NOT Qt6Quick_FOUND)
        message(FATAL_ERROR "OSMScout2 requires Qt6Quick")
    endif()

    add_subdirectory(OSMScout2)
endif()

# StyleEditor
if(OSMSCOUT_BUILD_TOOL_STYLEEDITOR)
    if(NOT ${OSMSCOUT_BUILD_CLIENT_QT})
        message(FATAL_ERROR "StyleEditor requires OSMSCOUT_BUILD_CLIENT_QT")
    endif()

    if(NOT Qt6Core_FOUND)
        message(FATAL_ERROR "StyleEditor requires Qt6Core")
    endif()

    if(NOT Qt6Gui_FOUND)
        message(FATAL_ERROR "StyleEditor requires Qt6Gui")
    endif()

    if(NOT Qt6Widgets_FOUND)
        message(FATAL_ERROR "StyleEditor requires Qt6Widgets")
    endif()

    if(NOT Qt6Qml_FOUND)
        message(FATAL_ERROR "StyleEditor requires Qt6Qml")
    endif()

    if(NOT Qt6Quick_FOUND)
        message(FATAL_ERROR "StyleEditor requires Qt6Quick")
    endif()

    add_subdirectory(StyleEditor)
endif()
