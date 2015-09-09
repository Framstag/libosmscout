cmake_minimum_required(VERSION 3.2)
project(Import)

include(FindPkgConfig)

pkg_check_modules(LIBOSMSCOUT REQUIRED libosmscout)
pkg_check_modules(LIBOSMSCOUTIMPORT REQUIRED libosmscout-import)

include_directories(${LIBOSMSCOUT_INCLUDE_DIRS}
                    ${LIBOSMSCOUTIMPORT_INCLUDE_DIRS})

link_directories(${LIBOSMSCOUT_LIBRARY_DIRS}
                 ${LIBOSMSCOUTIMPORT_LIBRARY_DIRS})

set(HEADER_FILES
    src/config.h)

set(SOURCE_FILES
    src/Import.cpp)

add_executable(Import ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(Import
                      ${LIBOSMSCOUT_LIBRARIES}
                      ${LIBOSMSCOUTIMPORT_LIBRARIES})

set_property(TARGET Import PROPERTY CXX_STANDARD 11)