set(SOURCE_FILES
	src/OSMScoutOpenGL.cpp
)

add_executable(OSMScoutOpenGL ${SOURCE_FILES})

set_target_properties(OSMScoutOpenGL PROPERTIES VERSION ${OSMSCOUT_LIBRARY_VERSION} SOVERSION ${OSMSCOUT_LIBRARY_VERSION} FOLDER "Tools")

target_include_directories(OSMScoutOpenGL PRIVATE
	Freetype::Freetype
)

target_link_libraries(OSMScoutOpenGL
	OSMScout::OSMScout
	OSMScout::Map
	OSMScout::MapOpenGL
	OpenGL::GL
	glfw
)

install(TARGETS OSMScoutOpenGL
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
