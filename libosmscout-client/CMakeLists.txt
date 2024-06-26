set(HEADER_FILES
  include/osmscoutclient/json/json.hpp
  include/osmscoutclient/json/json_fwd.hpp
  include/osmscoutclient/AdminRegionInfo.h
  include/osmscoutclient/DBInstance.h
  include/osmscoutclient/DBJob.h
  include/osmscoutclient/DBThread.h
  include/osmscoutclient/LocationInfo.h
  include/osmscoutclient/MapDirectory.h
  include/osmscoutclient/MapManager.h
  include/osmscoutclient/MapProvider.h
  include/osmscoutclient/ClientImportExport.h
  include/osmscoutclient/OnlineTileProvider.h
  include/osmscoutclient/POILookupModule.h
  include/osmscoutclient/Settings.h
  include/osmscoutclient/VoiceProvider.h
)

set(SOURCE_FILES
  src/osmscoutclient/DBInstance.cpp
  src/osmscoutclient/DBJob.cpp
  src/osmscoutclient/DBThread.cpp
  src/osmscoutclient/MapDirectory.cpp
  src/osmscoutclient/MapManager.cpp
  src/osmscoutclient/MapProvider.cpp
  src/osmscoutclient/OnlineTileProvider.cpp
  src/osmscoutclient/POILookupModule.cpp
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
	TARGET OSMScout::OSMScout OSMScout::Map
	SKIP_HEADER
)

if (TBB_FOUND)
	target_link_libraries(OSMScoutClient TBB::tbb)
endif()

if(OpenMP_CXX_FOUND)
	target_link_libraries(OSMScoutClient OpenMP::OpenMP_CXX)
endif()

if(MARISA_FOUND)
	target_include_directories(OSMScoutClient PUBLIC ${MARISA_INCLUDE_DIRS})
	target_link_libraries(OSMScoutClient ${MARISA_LIBRARIES})
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
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutclient DESTINATION include FILES_MATCHING REGEX "/.*\\.h|.*\\.hpp$" PATTERN "private" EXCLUDE)
else()
	set(_exclude PATTERN "private" EXCLUDE)
	foreach(ef ${EXCLUDE_HEADER})
		set(_exclude ${_exclude} PATTERN ${ef} EXCLUDE)
	endforeach()
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscoutclient DESTINATION include FILES_MATCHING REGEX "/.*\\.h|.*\\.hpp$" ${_exclude})
endif()
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscoutclient/ClientFeatures.h DESTINATION include/osmscout)
