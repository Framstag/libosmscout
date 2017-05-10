find_package(glfw3 3.2 REQUIRED)
find_package(OpenGL REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

string(TIMESTAMP OSMSCOUT_VERSION_STRING "%Y-%m-%d")
set(OSMSCOUT_USER_AGENT "OSMScout OpenGL demo app %1")

set(SOURCE_FILES
    src/OSMScoutOpenGL.cpp
)


add_executable(OSMScoutOpenGL ${SOURCE_FILES} ${HEADER_FILES} ${RESOURCE_FILES})

target_include_directories(OSMScoutOpenGL PRIVATE
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${OPENGL_INCLUDE_DIR}
)

target_link_libraries(OSMScoutOpenGL
        osmscout
        osmscout_map
        glfw
        ${OPENGL_gl_LIBRARY}
)
set_property(TARGET OSMScoutOpenGL PROPERTY CXX_STANDARD 11)
