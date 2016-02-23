set(OSMSCOUT_BUILD_TOOL_IMPORT ON CACHE INTERNAL "" FORCE)

set(SOURCE_FILES
    src/Import.cpp
)

add_executable(Import ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET Import PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/Config.h)
target_include_directories(Import PRIVATE 
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-import/include)
target_link_libraries(Import libosmscout libosmscout_import)
install(TARGETS Import
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
