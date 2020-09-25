set(HEADER_FILES_ROOT
        include/osmscout-test/TestImportExport.h
        include/osmscout-test/RegionList.h
        include/osmscout-test/PreprocessOLT.h)

set(HEADER_FILES_OLT
        include/osmscout-test/olt/Scanner.h
        include/osmscout-test/olt/Parser.h)

set(HEADER_FILES
        ${HEADER_FILES_ROOT}
        ${HEADER_FILES_OLT})

set(SOURCE_FILES
        src/osmscout-test/olt/Scanner.cpp
        src/osmscout-test/olt/Parser.cpp
        src/osmscout-test/RegionList.cpp
        src/osmscout-test/PreprocessOLT.cpp)

osmscout_library_project(
	NAME OSMScoutTest
	ALIAS Test
	OUTPUT_NAME "osmscout_test"
	SOURCE ${SOURCE_FILES}
	HEADER ${HEADER_FILES}
	TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout-test/TestFeatures.h.cmake
	FEATURE ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout-test/TestFeatures.h
	TARGET OSMScout::OSMScout OSMScout::Import
	SKIP_HEADER
)

set_source_files_properties(${HEADER_FILES_OLT}
        PROPERTIES MACOSX_PACKAGE_LOCATION Headers/ost)

if(APPLE AND OSMSCOUT_BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutTest PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION C
        MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
        #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
        PUBLIC_HEADER     "${HEADER_FILES_ROOT}"
        CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        OUTPUT_NAME "osmscout-test")
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout-test DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout-test/TestFeatures.h DESTINATION include/osmscout-test)
