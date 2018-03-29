set(OSMSCOUT_BUILD_TEST ON CACHE INTERNAL "" FORCE)

set(HEADER_FILES
		include/osmscout/private/TestImportExport.h
        include/osmscout/olt/Scanner.h
        include/osmscout/olt/Parser.h
        include/osmscout/RegionList.h
        include/osmscout/PreprocessOLT.h)

set(SOURCE_FILES
        src/osmscout/olt/Scanner.cpp
        src/osmscout/olt/Parser.cpp
        src/osmscout/RegionList.cpp
        src/osmscout/PreprocessOLT.cpp)

if(APPLE AND BUILD_FRAMEWORKS)
set(THE_TARGET_NAME "OSMScoutTest")
else()
set(THE_TARGET_NAME "osmscout_test")
endif()

if(IOS)
  add_library(${THE_TARGET_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})
else()
  add_library(${THE_TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif()

set_property(TARGET ${THE_TARGET_NAME} PROPERTY CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout/TestFeatures.h.cmake ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/TestFeatures.h)
create_private_config("${CMAKE_CURRENT_BINARY_DIR}/include/osmscout/private/Config.h" "osmscout-test")
target_include_directories(${THE_TARGET_NAME} PRIVATE
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
        ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-import/include
        )

if(APPLE AND BUILD_FRAMEWORKS)
  target_link_libraries(${THE_TARGET_NAME} OSMScout OSMScoutImport)
else()
  target_link_libraries(${THE_TARGET_NAME} osmscout osmscout_import)
endif()

if(${IOS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
endif()

if(APPLE AND BUILD_FRAMEWORKS)
set_target_properties(${THE_TARGET_NAME} PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION C
  MACOSX_FRAMEWORK_IDENTIFIER com.cmake.dynamicFramework
  #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
  PUBLIC_HEADER     "${HEADER_FILES}"
  CODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
)
endif()

target_compile_definitions(${THE_TARGET_NAME} PRIVATE -DOSMSCOUT_TEST_EXPORT_SYMBOLS)
install(TARGETS ${THE_TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/osmscout DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "private/Config.h" EXCLUDE)
install(FILES ${OSMSCOUT_BASE_DIR_BUILD}/include/osmscout/TestFeatures.h DESTINATION include/osmscout)
