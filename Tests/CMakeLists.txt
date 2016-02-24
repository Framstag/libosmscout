#---- CachePerformance
add_executable(CachePerformance src/CachePerformance.cpp)
set_property(TARGET CachePerformance PROPERTY CXX_STANDARD 11)
target_include_directories(CachePerformance PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CachePerformance libosmscout)
install(TARGETS CachePerformance RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- CalculateResolution
add_executable(CalculateResolution src/CalculateResolution.cpp)
set_property(TARGET CalculateResolution PROPERTY CXX_STANDARD 11)
target_include_directories(CalculateResolution PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CalculateResolution libosmscout)
install(TARGETS CalculateResolution RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- CoordinateEncoding
add_executable(CoordinateEncoding src/CoordinateEncoding.cpp)
set_property(TARGET CoordinateEncoding PROPERTY CXX_STANDARD 11)
target_include_directories(CoordinateEncoding PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(CoordinateEncoding libosmscout)
install(TARGETS CoordinateEncoding RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- NumberSetPerformance
add_executable(NumberSetPerformance src/NumberSetPerformance.cpp)
set_property(TARGET NumberSetPerformance PROPERTY CXX_STANDARD 11)
target_include_directories(NumberSetPerformance PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(NumberSetPerformance libosmscout)
install(TARGETS NumberSetPerformance RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- ReaderScannerPerformance
add_executable(ReaderScannerPerformance src/ReaderScannerPerformance.cpp)
set_property(TARGET ReaderScannerPerformance PROPERTY CXX_STANDARD 11)
target_include_directories(ReaderScannerPerformance PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(ReaderScannerPerformance libosmscout)
install(TARGETS ReaderScannerPerformance RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#---- ThreadedDatabase
if(${OSMSCOUT_BUILD_MAP})
	add_executable(ThreadedDatabase src/ThreadedDatabase.cpp)
	set_property(TARGET ThreadedDatabase PROPERTY CXX_STANDARD 11)
	target_include_directories(ThreadedDatabase PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include)
	target_link_libraries(ThreadedDatabase libosmscout libosmscout_map)
	install(TARGETS ThreadedDatabase RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
else()
	message("Skip ThreadedDatabase test libosmscout-map, is missing.")
endif()

#---- WorkQueue
add_executable(WorkQueue src/WorkQueue.cpp)
set_property(TARGET WorkQueue PROPERTY CXX_STANDARD 11)
target_include_directories(WorkQueue PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
target_link_libraries(WorkQueue libosmscout)
install(TARGETS WorkQueue RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
