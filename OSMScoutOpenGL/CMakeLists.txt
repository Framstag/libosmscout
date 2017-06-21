if(NOT ${OSMSCOUT_BUILD_MAP_OPENGL})
	message(SEND_ERROR "The OpenGL map drawing backend is required for OSMScoutOpenGL application")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

string(TIMESTAMP OSMSCOUT_VERSION_STRING "%Y-%m-%d")
set(OSMSCOUT_USER_AGENT "OSMScout OpenGL demo app %1")

set(SOURCE_FILES
		src/OSMScoutOpenGL.cpp)

add_executable(OSMScoutOpenGL ${SOURCE_FILES})

target_include_directories(OSMScoutOpenGL PRIVATE
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-opengl/include
		${OPENGL_INCLUDE_DIR}
		${GLEW_INCLUDE_DIRS}
		${GLFW_INCLUDE_DIR}
		)

if(MSVS OR MSYS OR MINGW)
	target_link_libraries(OSMScoutOpenGL
			osmscout
			osmscout_map
			osmscout_map_opengl
			${OPENGL_LIBRARIES}
			glfw3 # TODO: check if ${GLFW_LIBRARIES} is setup correctly on Windows
			)
else()
	target_link_libraries(OSMScoutOpenGL
			osmscout
			osmscout_map
			osmscout_map_opengl
			${OPENGL_LIBRARIES}
			${GLFW_LIBRARIES}
			)
endif()

set_property(TARGET OSMScoutOpenGL PROPERTY CXX_STANDARD 11)