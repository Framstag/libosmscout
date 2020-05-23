set(SOURCE_FILES
    src/PublicTransportMap.cpp
)

add_executable(PublicTransportMap ${SOURCE_FILES})

set_target_properties(PublicTransportMap PROPERTIES CXX_STANDARD 17)

target_link_libraries(PublicTransportMap OSMScout)

install(TARGETS PublicTransportMap
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
