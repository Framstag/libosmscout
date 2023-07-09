set(HEADER_FILES
    include/osmscoutclient/json/json.hpp
    include/osmscoutclient/json/json_fwd.hpp
    include/osmscoutclient/MapProvider.h
    include/osmscoutclient/ClientImportExport.h
    include/osmscoutclient/OnlineTileProvider.h
    include/osmscoutclient/Settings.h
    include/osmscoutclient/VoiceProvider.h
)

set(SOURCE_FILES
	src/osmscoutclient/MapProvider.cpp
  src/osmscoutclient/OnlineTileProvider.cpp
	src/osmscoutclient/Settings.cpp
	src/osmscoutclient/VoiceProvider.cpp
)

set(EXCLUDE_HEADER)

osmscout_library_project(
	NAME OSMScoutClient
	ALIAS Client
	OUTPUT_NAME "osmscout_client"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	INCLUDEDIR "osmscoutclient"
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutclient/ClientFeatures.h.cmake
	FEATURE ${CMAKE_CURRENT_BINARY_DIR}/include/osmscoutclient/ClientFeatures.h
	TARGET OSMScout::OSMScout
	SKIP_HEADER
)

if (HAVE_STD_EXECUTION)
	target_link_libraries(OSMScoutClient TBB::tbb)
endif()

if(OpenMP_CXX_FOUND)
	target_link_libraries(OSMScoutClient OpenMP::OpenMP_CXX)
endif()

if(APPLE AND OSMSCOUT_BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutClient PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION C
        MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
        #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
        PUBLIC_HEADER     "${HEADER_FILES}"
        CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        OUTPUT_NAME "OSMScoutClient")
endif()

if(WIN32)
	target_link_libraries(OSMScoutClient Ws2_32.lib)
endif()

if(NOT EXCLUDE_HEADER)
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutclient DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
else()
	set(_exclude PATTERN "private" EXCLUDE)
	foreach(ef ${EXCLUDE_HEADER})
		set(_exclude ${_exclude} PATTERN ${ef} EXCLUDE)
	endforeach()
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutclient DESTINATION include FILES_MATCHING PATTERN "*.h" ${_exclude})
endif()
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscoutclient/ClientFeatures.h DESTINATION include/osmscout)
