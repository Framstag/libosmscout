if(NOT ${OSMSCOUT_BUILD_IMPORT})
	message(SEND_ERROR "The main import library is required for BasemapImport application")
endif()

set(SOURCE_FILES
    src/BasemapImport.cpp
)

add_executable(BasemapImport ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET BasemapImport PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/Config.h)
target_include_directories(BasemapImport PRIVATE 
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-import/include)
target_link_libraries(BasemapImport osmscout_import)

if(BUILD_IMPORT_TOOL_FOR_DISTRIBUTION AND (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX OR CMAKE_COMPILER_IS_GNUCC))
  set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")  
endif()

install(TARGETS BasemapImport
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
