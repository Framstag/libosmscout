set(OSMSCOUT_BUILD_TEST ON CACHE INTERNAL "" FORCE)

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

if(IOS)
  add_library(OSMScoutTest STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(OSMScoutTest ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_target_properties(OSMScoutTest PROPERTIES
        CXX_STANDARD 17
        OUTPUT_NAME "osmscout_test")

set_source_files_properties(${HEADER_FILES_OLT}
        PROPERTIES MACOSX_PACKAGE_LOCATION Headers/ost)

target_include_directories(OSMScoutTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Public features Header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout-test/TestFeatures.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout-test/TestFeatures.h)
target_include_directories(OSMScoutTest PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)

# Private config header
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/privateinclude/osmscout-test/private/Config.h" "OSMScoutTest")
target_include_directories(OSMScoutTest PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/privateinclude)

target_link_libraries(OSMScoutTest OSMScout OSMScoutImport)

if(${IOS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
endif()

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

target_compile_definitions(OSMScoutTest PRIVATE -DOSMSCOUT_TEST_EXPORT_SYMBOLS)

install(TARGETS OSMScoutTest
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout-test DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/osmscout-test/TestFeatures.h DESTINATION include/osmscout-test)
