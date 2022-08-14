set(HEADER_FILES
		include/osmscoutmapopengl/MapOpenGLImportExport.h
		include/osmscoutmapopengl/OpenGLMapData.h
		include/osmscoutmapopengl/MapPainterOpenGL.h
		include/osmscoutmapopengl/Triangulate.h
		include/osmscoutmapopengl/PNGLoaderOpenGL.h
		include/osmscoutmapopengl/ShaderUtils.h
		include/osmscoutmapopengl/TextLoader.h
		include/poly2tri/poly2tri.h
		include/poly2tri/sweep/cdt.h
		include/poly2tri/sweep/sweep.h
		include/poly2tri/common/utils.h
		include/poly2tri/sweep/sweep.h
		include/poly2tri/sweep/advancing_front.h
		)

set(SOURCE_FILES
		src/osmscoutmapopengl/MapPainterOpenGL.cpp
		src/osmscoutmapopengl/Triangulate.cpp
		src/osmscoutmapopengl/PNGLoaderOpenGL.cpp
		src/osmscoutmapopengl/ShaderUtils.cpp
		src/osmscoutmapopengl/TextLoader.cpp
		src/poly2tri/sweep/cdt.cc
		src/poly2tri/common/shapes.cc
		src/poly2tri/sweep/sweep.cc
		src/poly2tri/sweep/sweep_context.cc
		src/poly2tri/sweep/advancing_front.cc)

set(SHADER_FILES
	data/shaders/AreaFragmentShader.frag
	data/shaders/TextVertexShader.vert
	data/shaders/QuadFragmentShader.frag
	data/shaders/QuadVertexShader.vert
	data/shaders/PathFragmentShader.frag
	data/shaders/AreaVertexShader.vert
	data/shaders/GroundFragmentShader.frag
	data/shaders/PathVertexShader.vert
	data/shaders/Projection.vert
	data/shaders/GroundVertexShader.vert
	data/shaders/TextFragmentShader.frag)

set(FONT_FILES
	data/fonts/LiberationSans-Regular.ttf
	data/fonts/LICENSE)

# install shaders
if(APPLE)
	set(SHADER_INSTALL_DIR "/Library/Application Support/osmscout/shaders")
	set(FONTS_INSTALL_DIR "/Library/Application Support/osmscout/fonts")
elseif(WIN32)
	set(SHADER_INSTALL_DIR "C:/ProgramData/osmscout/shaders")
	set(FONTS_INSTALL_DIR "C:/ProgramData/osmscout/fonts")
else() # Linux
	set(SHADER_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/osmscout/shaders")
	set(FONTS_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/osmscout/fonts")
endif()
install(FILES ${SHADER_FILES}
		DESTINATION ${SHADER_INSTALL_DIR})
install(FILES ${FONT_FILES}
	    DESTINATION ${FONTS_INSTALL_DIR})
set(DEFAULT_FONT_FILE "${FONTS_INSTALL_DIR}/LiberationSans-Regular.ttf")

osmscout_library_project(
	NAME OSMScoutMapOpenGL
	ALIAS MapOpenGL
	OUTPUT_NAME "osmscout_map_opengl"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDEDIR osmscoutmapopengl
	INCLUDES ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutmapopengl/MapOpenGLFeatures.h.cmake
	TARGET OSMScout::OSMScout OSMScout::Map ${OPENGL_LIBRARIES} GLEW::GLEW ${PNG_LIBRARIES} ${FREETYPE_LIBRARIES}
)
target_include_directories(OSMScoutMapOpenGL INTERFACE ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})

if(APPLE AND OSMSCOUT_BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutMapOpenGL PROPERTIES
  		FRAMEWORK TRUE
  		FRAMEWORK_VERSION C
  		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  		PUBLIC_HEADER     "${HEADER_FILES}"
  		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutMapOpenGL")
endif()
