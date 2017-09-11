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
		#include/osmscout/private/Config.h
		include/osmscout/private/MapOpenGLImportExport.h
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

if(APPLE)
  set(THE_TARGET_NAME "OSMScoutMapOpenGL") 
else()
  set(THE_TARGET_NAME "osmscout_map_opengl") 
endif()

if(IOS)
  add_library(${THE_TARGET_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(${THE_TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_property(TARGET ${THE_TARGET_NAME} PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/MapOpenGLFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapOpenGLFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "${THE_TARGET_NAME}")
target_include_directories(${THE_TARGET_NAME} PRIVATE
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
		${OPENGL_INCLUDE_DIR}
		${GLEW_INCLUDE_DIRS}
		${PNG_INCLUDE_DIRS}
		${FREETYPE_INCLUDE_DIRS}
		)

if(${IOS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
endif()

if(APPLE)
set_target_properties(${THE_TARGET_NAME} PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION C
  MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  PUBLIC_HEADER     "${HEADER_FILES}"
  CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
)
endif()

if(MSVS OR MSYS OR MINGW)
	target_link_libraries(${THE_TARGET_NAME}
			osmscout
			osmscout_map
			${OPENGL_LIBRARIES}
			${PNG_LIBRARIES}
			${FREETYPE_LIBRARIES}
			glew32 # TODO: ${GLEW_LIBRARIES} is not setup correctly on Windows?
			)
elseif(APPLE)
	target_link_libraries(${THE_TARGET_NAME}
			OSMScout
			OSMScoutMap
			${OPENGL_LIBRARIES}
			${GLEW_LIBRARIES}
			${PNG_LIBRARIES}
			${FREETYPE_LIBRARIES}
			)
else()
	target_link_libraries(${THE_TARGET_NAME}
			osmscout
			osmscout_map
			${OPENGL_LIBRARIES}
			${GLEW_LIBRARIES}
			${PNG_LIBRARIES}
			${FREETYPE_LIBRARIES}
			)
endif()
target_compile_definitions(${THE_TARGET_NAME} PRIVATE -DOSMSCOUT_MAP_OPENGL_EXPORT_SYMBOLS)
install(TARGETS ${THE_TARGET_NAME}
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
		FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/MapOpenGLFeatures.h DESTINATION include/osmscout)
