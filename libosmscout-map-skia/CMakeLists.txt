set(HEADER_FILES
    include/osmscoutmapskia/MapSkiaImportExport.h
    include/osmscoutmapskia/MapSkiaFeatures.h
    include/osmscoutmapskia/MapPainterSkia.h
    include/osmscoutmapskia/SymbolRendererSkia.h
)

set(SOURCE_FILES
    src/osmscoutmapskia/MapPainterSkia.cpp
    src/osmscoutmapskia/SymbolRendererSkia.cpp
)

osmscout_library_project(
	NAME OSMScoutMapSkia
	ALIAS MapSkia
	OUTPUT_NAME "osmscout_map_skia"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDEDIR "osmscoutmapskia"
	INCLUDES ${SKIA_INCLUDE_DIRS}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutmapskia/MapSkiaFeatures.h.cmake
	TARGET OSMScout::OSMScout OSMScout::Map Skia::skia
)

# Find fontconfig for Skia font manager
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(FONTCONFIG fontconfig)
  if(FONTCONFIG_FOUND)
    target_include_directories(OSMScoutMapSkia PRIVATE ${FONTCONFIG_INCLUDE_DIRS})
    target_link_libraries(OSMScoutMapSkia ${FONTCONFIG_LIBRARIES})
  endif()
endif()
target_include_directories(OSMScoutMapSkia INTERFACE ${SKIA_INCLUDE_DIRS})
target_include_directories(OSMScoutMapSkia PRIVATE ${CMAKE_SOURCE_DIR}/include-patch)
target_include_directories(OSMScoutMapSkia INTERFACE ${CMAKE_SOURCE_DIR}/include-patch)

# Skia SVG module (optional, for SVG icon loading)
if(OSMSCOUT_HAVE_SKIA_SVG)
  target_compile_definitions(OSMScoutMapSkia PRIVATE OSMSCOUT_HAVE_SKIA_SVG)
endif()

if(APPLE AND OSMSCOUT_BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutMapSkia PROPERTIES
  		FRAMEWORK TRUE
  		FRAMEWORK_VERSION C
  		MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  		PUBLIC_HEADER     "${HEADER_FILES}"
  		CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
		OUTPUT_NAME "OSMScoutMapSkia")
endif()
