cmake_minimum_required(VERSION 3.2)
project(Demos)

include(FindPkgConfig)

pkg_check_modules(LIBOSMSCOUT REQUIRED libosmscout)

include_directories(${LIBOSMSCOUT_INCLUDE_DIRS})

link_directories(${LIBOSMSCOUT_LIBRARY_DIRS})

#---- CachePerformance

add_executable(CachePerformance src/CachePerformance.cpp)

target_link_libraries(CachePerformance
        ${LIBOSMSCOUT_LIBRARIES})

set_property(TARGET CachePerformance PROPERTY CXX_STANDARD 11)


#---- CalculateResolution

add_executable(CalculateResolution src/CalculateResolution.cpp)

target_link_libraries(CalculateResolution
        ${LIBOSMSCOUT_LIBRARIES})

set_property(TARGET CalculateResolution PROPERTY CXX_STANDARD 11)

#---- CoordinateEncoding

add_executable(CoordinateEncoding src/CoordinateEncoding.cpp)

target_link_libraries(CoordinateEncoding
        ${LIBOSMSCOUT_LIBRARIES})

set_property(TARGET CoordinateEncoding PROPERTY CXX_STANDARD 11)

#---- NumberSetPerformance

add_executable(NumberSetPerformance src/NumberSetPerformance.cpp)

target_link_libraries(NumberSetPerformance
        ${LIBOSMSCOUT_LIBRARIES})

set_property(TARGET NumberSetPerformance PROPERTY CXX_STANDARD 11)

#---- ReaderScannerPerformance

add_executable(ReaderScannerPerformance src/ReaderScannerPerformance.cpp)

target_link_libraries(ReaderScannerPerformance
        ${LIBOSMSCOUT_LIBRARIES})

set_property(TARGET ReaderScannerPerformance PROPERTY CXX_STANDARD 11)

#---- ThreadedDatabase

add_executable(ThreadedDatabase src/ThreadedDatabase.cpp)

target_link_libraries(ThreadedDatabase
        ${LIBOSMSCOUT_LIBRARIES})

set_property(TARGET ThreadedDatabase PROPERTY CXX_STANDARD 11)

#---- WorkQueue

add_executable(WorkQueue src/WorkQueue.cpp)

target_link_libraries(WorkQueue
        ${LIBOSMSCOUT_LIBRARIES})

set_property(TARGET WorkQueue PROPERTY CXX_STANDARD 11)
