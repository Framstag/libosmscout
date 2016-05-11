if(NOT OPENGL_FOUND)
	message(SEND_ERROR "OpenGL is required for OpenGL map drawing backend")
endif()
if(NOT GLUT_FOUND)
	message(SEND_ERROR "Glut is required for OpenGL map drawing backend")
endif()
if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for OpenGL map drawing backend")
endif()

set(HEADER_FILES
    #include/osmscout/private/Config.h
    include/osmscout/private/MapOpenGLImportExport.h
    #include/osmscout/MapOpenGLFeatures.h
    include/osmscout/MapPainterOpenGL.h
)

set(SOURCE_FILES
    src/osmscout/MapPainterOpenGL.cpp
)

add_library(osmscout_map_opengl SHARED ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET osmscout_map_opengl PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapOpenGLFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapOpenGLFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "osmscout-map-opengl")
target_include_directories(osmscout_map_opengl PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${OPENGL_INCLUDE_DIR}
		${GLUT_INCLUDE_DIR})
target_link_libraries(osmscout_map_opengl
		osmscout
		osmscout_map
		${OPENGL_LIBRARIES}
		${GLUT_LIBRARIES})
target_compile_definitions(osmscout_map_opengl PRIVATE -DOSMSCOUT_MAP_OPENGL_EXPORT_SYMBOLS)
install(TARGETS osmscout_map_opengl
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapOpenGLFeatures.h DESTINATION include/osmscout)
