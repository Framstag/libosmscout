# Dependencies:
# OSMSCOUT_BUILD_MAP_QT: OSMSCOUT_BUILD_MAP, Qt(Gui, Svg)
# OSMSCOUT_BUILD_CLIENT_QT: OSMSCOUT_BUILD_MAP_QT, Qt(Core, Gui, Quick, Multimedia)
# OSMSCOUT_BUILD_TOOL_OSMSCOUT2: OSMSCOUT_BUILD_CLIENT_QT, Qt(Core, Gui, Widgets, Qml, Quick)
# OSMSCOUT_BUILD_TOOL_STYLEEDITOR: OSMSCOUT_BUILD_CLIENT_QT, Qt(Core, Gui, Widgets, Qml, Quick)

option(OSMSCOUT_BUILD_MAP_QT "Enable build of Qt map drawing backend" OFF)
option(OSMSCOUT_BUILD_CLIENT_QT "Find newest Qt in respect to QT_VERSION_MAJOR and QT_VERSION_MINOR" OFF)
option(OSMSCOUT_BUILD_TOOL_OSMSCOUT2 "Enable build of OSMSCout2 demo" OFF)
option(OSMSCOUT_BUILD_TOOL_STYLEEDITOR "Enable build of StyleEditor application" OFF)

# Find newest Qt in respect to QT_VERSION_MAJOR and QT_VERSION_MINOR
set(QT_VERSION_MAJOR "" CACHE STRING "")
set(QT_VERSION_MINOR "" CACHE STRING "")

if (OSMSCOUT_BUILD_MAP_QT OR OSMSCOUT_BUILD_CLIENT_QT)
    if(NOT ${QT_VERSION_MAJOR} OR ${QT_VERSION_MAJOR} STREQUAL "")
        set(QT_VERSION_MAJOR "6" CACHE STRING "" FORCE)
        set(QT_VERSION_MINOR "" CACHE STRING "" FORCE)
    endif()

    if(QT_VERSION_MAJOR STREQUAL "6")
        include(feature-qt6)
    elseif(QT_VERSION_MAJOR STREQUAL "5")
        include(feature-qt5)
    endif()
endif()
