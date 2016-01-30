cmake_minimum_required(VERSION 3.2)
project(DumpData)

set(SOURCE_FILES
    src/DumpData.cpp
)

add_executable(DumpData ${SOURCE_FILES})
set_property(TARGET DumpData PROPERTY CXX_STANDARD 11)
target_include_directories(DumpData PRIVATE 
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-import/include)
target_link_libraries(DumpData libosmscout libosmscout_import)
install(TARGETS DumpData
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
