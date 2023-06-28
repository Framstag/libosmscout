# Dependencies:
# OSMSCOUT_BUILD_MAP_QT: OSMSCOUT_BUILD_MAP, Qt(Gui, Svg)
# OSMSCOUT_BUILD_CLIENT_QT: OSMSCOUT_BUILD_MAP_QT, Qt(Core, Gui, Quick, Multimedia)
# OSMSCOUT_BUILD_TOOL_OSMSCOUT2: OSMSCOUT_BUILD_CLIENT_QT, Qt(Core, Gui, Widgets, Qml, Quick)
# OSMSCOUT_BUILD_TOOL_STYLEEDITOR: OSMSCOUT_BUILD_CLIENT_QT, Qt(Core, Gui, Widgets, Qml, Quick)

# Qt version selection
# Two methods
# Method 1: use the built-in rules to select a qt version, see ENABLE_QT_WITH_BUILT
# Method 2: hard coded: OSMSCOUT_BUILD_MAP_QT, OSMSCOUT_BUILD_CLIENT_QT,
#                       OSMSCOUT_BUILD_TOOL_OSMSCOUT2, OSMSCOUT_BUILD_TOOL_STYLEEDITOR,
#                       QT_MAJOR_VERSION,QT_MINOR_VERSION

option(OSMSCOUT_BUILD_MAP_QT "Enable build of Qt map drawing backend" ON)
option(OSMSCOUT_BUILD_CLIENT_QT "Enable build of Qt client" ON)
option(OSMSCOUT_BUILD_TOOL_OSMSCOUT2 "Enable build of OSMSCout2 demo" ON)
option(OSMSCOUT_BUILD_TOOL_STYLEEDITOR "Enable build of StyleEditor application" ON)

# ENABLE_QT_BY_BUILT_IN_RULES OFF: find the newest qt in respect to QT_MAJOR_VERSION and QT_MINOR_VERSION
# ENABLE_QT_BY_BUILT_IN_RULES ON: find qt by built-in rules in priority as below:
# 1. explicit (OSMSCOUT_BUILD_MAP_QT, OSMSCOUT_BUILD_CLIENT_QT, QT_MAJOR_VERSION, QT_MINOR_VERSION)
# 2. Qt5
# 3. Qt6
# *. OSMSCOUT_BUILD_TOOL_OSMSCOUT2 and OSMSCOUT_BUILD_TOOL_STYLEEDITOR are enabled if possible
option(ENABLE_QT_BY_BUILT_IN_RULES "Enable Qt with built in rules" ON)

# Find newest Qt in respect to QT_MAJOR_VERSION and QT_MINOR_VERSION
set(QT_MAJOR_VERSION "" CACHE STRING "")
set(QT_MINOR_VERSION "" CACHE STRING "")

if (NOT OSMSCOUT_BUILD_MAP_QT AND NOT OSMSCOUT_BUILD_CLIENT_QT)
    return()
endif()

if(NOT ${QT_MAJOR_VERSION} OR ${QT_MAJOR_VERSION} STREQUAL "")
    if(${ENABLE_QT_WITH_BUILT_IN_RULES})
        # Qt5    
        include(feature-qt5)

        if(${OSMSCOUT_BUILD_MAP_QT_FOR_QT5_ENABLED} AND ${OSMSCOUT_BUILD_CLIENT_QT_FOR_QT5_ENABLED})
            if(NOT ${OSMSCOUT_BUILD_TOOL_OSMSCOUT2_FOR_QT5_ENABLED})
                message(WARNING "OSMSCOUT_BUILD_TOOL_OSMSCOUT2 failed to be enabled")
            endif()

            if(NOT ${OSMSCOUT_BUILD_TOOL_STYLEEDITOR_FOR_QT5_ENABLED})
                message(WARNING "OSMSCOUT_BUILD_TOOL_STYLEEDITOR failed to be enabled")
            endif()

            return()
        endif()

        if (NOT ${OSMSCOUT_BUILD_MAP_QT_FOR_QT5_ENABLED})
            message(NOTICE "OSMSCOUT_BUILD_MAP_QT(Qt5) failed to be enabled")
        endif()

        if (NOT ${OSMSCOUT_BUILD_CLIENT_QT_FOR_QT5_ENABLED})
            message(NOTICE "OSMSCOUT_BUILD_CLIENT_QT(Qt5) failed to be enabled")
        endif()

        # Qt6
        include(feature-qt6)
        if(${OSMSCOUT_BUILD_MAP_QT_FOR_QT6_ENABLED} AND ${OSMSCOUT_BUILD_CLIENT_QT_FOR_QT6_ENABLED})
            if(NOT ${OSMSCOUT_BUILD_TOOL_OSMSCOUT2_FOR_QT6_ENABLED})
                message(WARNING "OSMSCOUT_BUILD_TOOL_OSMSCOUT2(Qt6) failed to be enabled")
            endif()
            
            if(NOT ${OSMSCOUT_BUILD_TOOL_STYLEEDITOR_FOR_QT6_ENABLED})
                message(WARNING "OSMSCOUT_BUILD_TOOL_STYLEEDITOR(Qt6) failed to be enabled")
            endif()

            return()
        endif()

        if (NOT ${OSMSCOUT_BUILD_MAP_QT_FOR_QT6_ENABLED})
            message(FATAL_ERROR "OSMSCOUT_BUILD_MAP_QT(Qt6) failed to be enabled")
        endif()

        if (NOT ${OSMSCOUT_BUILD_CLIENT_QT_FOR_QT6_ENABLED})
            message(FATAL_ERROR "OSMSCOUT_BUILD_CLIENT_QT(Qt6) failed to be enabled")
        endif()

        return()
    else()
        set(QT_MAJOR_VERSION "6" CACHE STRING "" FORCE)
        set(QT_MINOR_VERSION "" CACHE STRING "" FORCE)
    endif()
endif()

if(${QT_MAJOR_VERSION} STREQUAL "6")
    include(feature-qt6)

    if(NOT ${OSMSCOUT_BUILD_MAP_QT_FOR_QT6_ENABLED})
        message(FATAL_ERROR "OSMSCOUT_BUILD_MAP_QT(Qt6) failed to be enabled")
    endif()

    if(NOT ${OSMSCOUT_BUILD_CLIENT_QT_FOR_QT6_ENABLED})
        message(FATAL_ERROR "OSMSCOUT_BUILD_CLIENT(Qt6) failed to be enabled")
    endif()

    if(NOT ${OSMSCOUT_BUILD_TOOL_OSMSCOUT2_FOR_QT6_ENABLED})
        message(WARNING "OSMSCOUT_BUILD_TOOL_OSMSCOUT2(Qt6) failed to be enabled")
    endif()

    if(NOT ${OSMSCOUT_BUILD_TOOL_STYLEEDITOR_FOR_QT6_ENABLED})
        message(WARNING "OSMSCOUT_BUILD_TOOL_STYLEEDITOR(Qt6) failed to be enabled")
    endif()
elseif(${QT_MAJOR_VERSION} STREQUAL "5")
    include(feature-qt5)

    if(NOT ${OSMSCOUT_BUILD_MAP_QT_FOR_QT5_ENABLED})
        message(FATAL_ERROR "OSMSCOUT_BUILD_MAP_QT(Qt5) failed to be enabled")
    endif()

    if(NOT ${OSMSCOUT_BUILD_CLIENT_QT_FOR_QT5_ENABLED})
        message(FATAL_ERROR "OSMSCOUT_BUILD_CLIENT(Qt5) failed to be enabled")
    endif()

    if(NOT ${OSMSCOUT_BUILD_TOOL_OSMSCOUT2_FOR_QT5_ENABLED})
        message(WARNING "OSMSCOUT_BUILD_TOOL_OSMSCOUT2(Qt5) failed to be enabled")
    endif()

    if(NOT ${OSMSCOUT_BUILD_TOOL_STYLEEDITOR_FOR_QT5_ENABLED})
        message(WARNING "OSMSCOUT_BUILD_TOOL_STYLEEDITOR(Qt5) failed to be enabled")
    endif()
endif()
