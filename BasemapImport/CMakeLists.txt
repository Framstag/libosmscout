set(OSMSCOUT_BUILD_IMPORT ON CACHE INTERNAL "" FORCE)

set(SOURCE_FILES
    src/BasemapImport.cpp
)

add_executable(BasemapImport ${SOURCE_FILES} ${HEADER_FILES})

set_target_properties(BasemapImport PROPERTIES VERSION ${OSMSCOUT_LIBRARY_VERSION} SOVERSION ${OSMSCOUT_LIBRARY_VERSION} FOLDER "Tools")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/Config.h)

target_include_directories(BasemapImport PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

target_link_libraries(BasemapImport OSMScout::Import)

if(OSMSCOUT_BUILD_IMPORT_TOOL_FOR_DISTRIBUTION AND (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX OR CMAKE_COMPILER_IS_GNUCC))
  set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
endif()
if(WIN32)
	create_win32_tool_resource(BasemapImport)
endif()

install(TARGETS BasemapImport
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
