set(SOURCE_FILES
    src/MCPServer.cpp
    src/LocationDescriptionMapper.cpp
    src/LocationDescriptionMapper.h
    src/VersionTool.cpp
    src/VersionTool.h
    src/LocationDescriptionTool.cpp
    src/LocationDescriptionTool.h
)

add_executable(MCPServer ${SOURCE_FILES})

set_target_properties(MCPServer PROPERTIES
    VERSION ${OSMSCOUT_LIBRARY_VERSION}
    SOVERSION ${OSMSCOUT_LIBRARY_VERSION}
    FOLDER "Tools"
)

target_link_libraries(MCPServer PRIVATE
    OSMScout::OSMScout
    nlohmann_json::nlohmann_json
)

if(WIN32)
    create_win32_tool_resource(MCPServer)
endif()

install(TARGETS MCPServer
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    BUNDLE DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)