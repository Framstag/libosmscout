set(SOURCE_FILES
    src/PublicTransportMap.cpp
)

add_executable(PublicTransportMap ${SOURCE_FILES})

set_target_properties(PublicTransportMap PROPERTIES VERSION ${OSMSCOUT_LIBRARY_VERSION} SOVERSION ${OSMSCOUT_LIBRARY_VERSION} FOLDER "Tools")

target_link_libraries(PublicTransportMap OSMScout::OSMScout)

if(WIN32)
	create_win32_tool_resource(PublicTransportMap)
endif()

install(TARGETS PublicTransportMap
        RUNTIME DESTINATION bin
		BUNDLE DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
