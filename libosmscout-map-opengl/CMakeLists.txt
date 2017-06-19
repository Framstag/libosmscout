if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for OpenGL map drawing backend")
endif()

set(HEADER_FILES
		#include/osmscout/private/Config.h
		include/osmscout/private/MapOpenGLImportExport.h
		include/osmscout/OpenGLMapData.h
		include/osmscout/MapPainterOpenGL.h
		include/osmscout/Triangulate.h
		include/poly2tri/poly2tri.h
		include/poly2tri/sweep/cdt.h
		include/poly2tri/sweep/sweep.h
		include/poly2tri/common/utils.h
		include/poly2tri/sweep/sweep.h
		include/poly2tri/sweep/advancing_front.h
		)

set(SOURCE_FILES
		src/osmscout/MapPainterOpenGL.cpp
		src/osmscout/Triangulate.cpp
		include/poly2tri/sweep/cdt.cc
		include/poly2tri/common/shapes.cc
		include/poly2tri/sweep/sweep.cc
		include/poly2tri/sweep/sweep_context.cc
		include/poly2tri/sweep/advancing_front.cc
		)

add_library(osmscout_map_opengl ${SOURCE_FILES} ${HEADER_FILES})
set_property(TARGET osmscout_map_opengl PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapOpenGLFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapOpenGLFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "osmscout-map-opengl")
target_include_directories(osmscout_map_opengl PRIVATE
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${OPENGL_INCLUDE_DIR}
		${GLEW_INCLUDE_DIRS}
		)

if(MSVS OR MSYS OR MINGW)
	target_link_libraries(osmscout_map_opengl
			osmscout
			osmscout_map
			${OPENGL_LIBRARIES}
			glew32 # TODO: ${GLEW_LIBRARIES} is not setup correctly on Windows?
			)
else()
	target_link_libraries(osmscout_map_opengl
			osmscout
			osmscout_map
			${OPENGL_LIBRARIES}
			${GLEW_LIBRARIES}
			)
endif()
target_compile_definitions(osmscout_map_opengl PRIVATE -DOSMSCOUT_MAP_OPENGL_EXPORT_SYMBOLS)
install(TARGETS osmscout_map_opengl
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapOpenGLFeatures.h DESTINATION include/osmscout)