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

osmscout_library_project(
	NAME OSMScoutMapOpenGL
	ALIAS MapOpenGL
	OUTPUT_NAME "osmscout_map_opengl"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDES ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapOpenGLFeatures.h.cmake
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
