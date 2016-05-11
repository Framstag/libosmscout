if(NOT ${OSMSCOUT_BUILD_IMPORT})
	message(SEND_ERROR "The main import library is required for DumpData application")
endif()

set(SOURCE_FILES
    src/DumpData.cpp
)

add_executable(DumpData ${SOURCE_FILES})
set_property(TARGET DumpData PROPERTY CXX_STANDARD 11)
target_include_directories(DumpData PRIVATE 
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-import/include)
target_link_libraries(DumpData osmscout osmscout_import)
install(TARGETS DumpData
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
