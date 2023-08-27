set(SOURCE_FILES
    src/DumpData.cpp
)

add_executable(DumpData ${SOURCE_FILES})

set_target_properties(DumpData PROPERTIES VERSION ${OSMSCOUT_LIBRARY_VERSION} SOVERSION ${OSMSCOUT_LIBRARY_VERSION} FOLDER "Tools")

target_link_libraries(DumpData OSMScout::OSMScout)

if(WIN32)
	create_win32_tool_resource(DumpData)
endif()

install(TARGETS DumpData
        RUNTIME DESTINATION bin
		BUNDLE DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
