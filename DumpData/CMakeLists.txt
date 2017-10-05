set(SOURCE_FILES
    src/DumpData.cpp
)

add_executable(DumpData ${SOURCE_FILES})
set_property(TARGET DumpData PROPERTY CXX_STANDARD 11)
target_include_directories(DumpData PRIVATE
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
if(APPLE)
  target_link_libraries(DumpData OSMScout)
else()
  target_link_libraries(DumpData osmscout)
endif()
install(TARGETS DumpData
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
