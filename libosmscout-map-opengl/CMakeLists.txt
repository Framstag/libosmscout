set(HEADER_FILES
        include/osmscout/MapOpenGLImportExport.h
		include/osmscout/OpenGLMapData.h
		include/osmscout/MapPainterOpenGL.h
		include/osmscout/Triangulate.h
		include/osmscout/PNGLoaderOpenGL.h
		include/osmscout/TextLoader.h
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
		src/osmscout/PNGLoaderOpenGL.cpp
		src/osmscout/TextLoader.cpp
		src/poly2tri/sweep/cdt.cc
		src/poly2tri/common/shapes.cc
		src/poly2tri/sweep/sweep.cc
		src/poly2tri/sweep/sweep_context.cc
		src/poly2tri/sweep/advancing_front.cc)

add_library(OSMScoutMapOpenGL ${SOURCE_FILES} ${HEADER_FILES})
add_library(OSMScout::MapOpenGL ALIAS OSMScoutMapOpenGL)
set_target_properties(OSMScoutMapOpenGL PROPERTIES
	OUTPUT_NAME "osmscout_map_opengl"
	VERSION ${OSMSCOUT_LIBRARY_VERSION}
	SOVERSION ${OSMSCOUT_LIBRARY_VERSION}
	FOLDER "libosmscout"
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapOpenGLFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapOpenGLFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutMapOpenGL")
target_include_directories(OSMScoutMapOpenGL PRIVATE
	include
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/privateinclude
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${OPENGL_INCLUDE_DIR}
	${GLEW_INCLUDE_DIRS}
	${GLM_INCLUDE_DIRS}
	${PNG_INCLUDE_DIRS}
	${FREETYPE_INCLUDE_DIRS}
)
target_include_directories(OSMScoutMapOpenGL INTERFACE ${CMAKE_SOURCE_DIR}/libosmscout-map-opengl/include ${CMAKE_BINARY_DIR}/libosmscout-map-opengl/include ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(OSMScoutMapOpenGL OSMScout::OSMScout OSMScout::Map ${OPENGL_LIBRARIES} GLEW::GLEW ${PNG_LIBRARIES} ${FREETYPE_LIBRARIES})

if(APPLE AND BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutMapOpenGL PROPERTIES
  		FRAMEWORK TRUE
  		FRAMEWORK_VERSION C
  		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  		PUBLIC_HEADER     "${HEADER_FILES}"
  		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutMapOpenGL")
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
	create_win32_dll_resource(OSMScoutMapOpenGL "osmscout_map_opengl")
endif()

install(TARGETS OSMScoutMapOpenGL
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
		FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapOpenGLFeatures.h DESTINATION include/osmscout)
