if(NOT ${OSMSCOUT_BUILD_MAP})
	message(SEND_ERROR "The main map drawing interface is required for OpenGL map drawing backend")
endif()
if(NOT PNG_FOUND)
	message(SEND_ERROR "libpng is required for opengl map drawing backend")
endif()
if(NOT FREETYPE_FOUND)
	message(SEND_ERROR "freetype is required for opengl map drawing backend")
endif()

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

if(IOS)
  add_library(OSMScoutMapOpenGL STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(OSMScoutMapOpenGL ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_target_properties(OSMScoutMapOpenGL PROPERTIES
		CXX_STANDARD 17
		OUTPUT_NAME "OSMScoutMapOpenGL")

target_include_directories(OSMScoutMapOpenGL PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${OPENGL_INCLUDE_DIR}
		${GLEW_INCLUDE_DIRS}
		${GLM_INCLUDE_DIRS}
		${PNG_INCLUDE_DIRS}
		${FREETYPE_INCLUDE_DIRS})

# Public features Header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapOpenGLFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapOpenGLFeatures.h)
target_include_directories(OSMScoutMapOpenGL PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)

# Private config header
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout/private/Config.h" "OSMScoutMapOpenGL")
target_include_directories(OSMScoutMapOpenGL PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/privateinclude)

if(${IOS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
endif()

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

target_link_libraries(OSMScoutMapOpenGL
		OSMScout
		OSMScoutMap
		${OPENGL_LIBRARIES}
		GLEW::GLEW
		${PNG_LIBRARIES}
		${FREETYPE_LIBRARIES}
		)

target_compile_definitions(OSMScoutMapOpenGL PRIVATE -DOSMSCOUT_MAP_OPENGL_EXPORT_SYMBOLS)

install(TARGETS OSMScoutMapOpenGL
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
		FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/MapOpenGLFeatures.h DESTINATION include/osmscout)
