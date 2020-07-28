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

add_library(OSMScoutTest ${SOURCE_FILES} ${HEADER_FILES})
add_library(OSMScout::Test ALIAS OSMScoutTest)
set_target_properties(OSMScoutTest PROPERTIES
	OUTPUT_NAME "osmscout_test"
	VERSION ${OSMSCOUT_LIBRARY_VERSION}
	SOVERSION ${OSMSCOUT_LIBRARY_VERSION}
	FOLDER "libosmscout"
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout-test/TestFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout-test/TestFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout-test/private/Config.h" "OSMScoutTest")
target_include_directories(OSMScoutTest PRIVATE
	include
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/privateinclude
	${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(OSMScoutTest INTERFACE ${CMAKE_SOURCE_DIR}/libosmscout-test/include ${CMAKE_BINARY_DIR}/libosmscout-test/include)
target_link_libraries(OSMScoutTest OSMScout::Import)

set_source_files_properties(${HEADER_FILES_OLT}
        PROPERTIES MACOSX_PACKAGE_LOCATION Headers/ost)

if(APPLE AND BUILD_FRAMEWORKS)
	set_target_properties(OSMScoutTest PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION C
        MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
        #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
        PUBLIC_HEADER     "${HEADER_FILES_ROOT}"
        CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        OUTPUT_NAME "osmscout-test")
endif()

install(TARGETS OSMScoutTest
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout-test DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout-test/TestFeatures.h DESTINATION include/osmscout-test)
