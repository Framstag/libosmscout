set(OSMSCOUT_BUILD_IMPORT ON CACHE INTERNAL "" FORCE)

set(SOURCE_FILES
    src/Import.cpp
)

add_executable(Import ${SOURCE_FILES} ${HEADER_FILES})

set_target_properties(Import PROPERTIES VERSION ${OSMSCOUT_LIBRARY_VERSION} SOVERSION ${OSMSCOUT_LIBRARY_VERSION} FOLDER "Tools")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/Config.h)

target_include_directories(Import PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

target_link_libraries(Import OSMScout::Import)

if(OSMSCOUT_BUILD_IMPORT_TOOL_FOR_DISTRIBUTION AND (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX OR CMAKE_COMPILER_IS_GNUCC))
  set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
endif()
if(WIN32)
	create_win32_tool_resource(Import)
endif()

install(TARGETS Import
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
