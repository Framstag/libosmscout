set(SOURCE_FILES
	src/OSMScoutOpenGL.cpp
)

add_executable(OSMScoutOpenGL ${SOURCE_FILES})

set_target_properties(OSMScoutOpenGL PROPERTIES VERSION ${OSMSCOUT_LIBRARY_VERSION} SOVERSION ${OSMSCOUT_LIBRARY_VERSION} FOLDER "Tools")

target_include_directories(OSMScoutOpenGL PRIVATE
	${GLEW_INCLUDE_DIRS}
	${GLFW_INCLUDE_DIR}
	Freetype::Freetype
)

target_link_libraries(OSMScoutOpenGL
	OSMScout::OSMScout
	OSMScout::Map
	OSMScout::MapOpenGL
	OpenGL::GL
	${GLFW_LIBRARIES}
)

install(TARGETS OSMScoutOpenGL
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
