set(SOURCE_FILES
    src/DumpData.cpp
)

add_executable(DumpData ${SOURCE_FILES})

set_target_properties(DumpData PROPERTIES CXX_STANDARD 14)

target_link_libraries(DumpData OSMScout)

install(TARGETS DumpData
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
