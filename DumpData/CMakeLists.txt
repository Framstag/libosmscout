cmake_minimum_required(VERSION 3.2)
project(DumpData)

include(FindPkgConfig)

pkg_check_modules(LIBOSMSCOUT REQUIRED libosmscout)

include_directories(${LIBOSMSCOUT_INCLUDE_DIRS})

link_directories(${LIBOSMSCOUT_LIBRARY_DIRS})

set(HEADER_FILES
    src/config.h)

set(SOURCE_FILES
    src/DumpData.cpp)

add_executable(DumpData ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(DumpData
                      ${LIBOSMSCOUT_LIBRARIES}
                      ${LIBOSMSCOUTIMPORT_LIBRARIES})

set_property(TARGET DumpData PROPERTY CXX_STANDARD 11)