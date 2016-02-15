cmake_minimum_required(VERSION 3.2)
project(libosmscout_binding)

if(NOT SWIG_FOUND)
  message(FATAL_ERROR "SWIG is needed to create the java binding.")
else()
  find_package(JNI REQUIRED)
endif()
include(${SWIG_USE_FILE})
set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR}/Java/src/main/java/net/sf/libosmscout/osmscout)
file(MAKE_DIRECTORY ${CMAKE_SWIG_OUTDIR})
set(CMAKE_SWIG_FLAGS -c++ -package net.sf.libosmscout.osmscout -features autodoc=1 -I${CMAKE_CURRENT_SOURCE_DIR}/src/swig -I${CMAKE_CURRENT_SOURCE_DIR}/../libosmscout/include -outdir ${CMAKE_SWIG_OUTDIR})
if(MSVC)
  set(CMAKE_SWIG_FLAGS ${CMAKE_SWIG_FLAGS} -Fmicrosoft)
endif()
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/swig/osmscout/libosmscout.i PROPERTIES CPLUSPLUS ON)
swig_add_module(libosmscout_binding Java ${CMAKE_CURRENT_SOURCE_DIR}/src/swig/osmscout/libosmscout.i)
swig_link_libraries(libosmscout_binding libosmscout ${JNI_LIBRARIES})
target_include_directories(libosmscout_binding PRIVATE 
		include
		${CMAKE_CURRENT_BINARY_DIR}/include
		${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include
		${JNI_INCLUDE_DIRS}
		${JAVA_INCLUDE_PATH})
target_link_libraries(libosmscout_binding
		libosmscout
		${JNI_LIBRARIES})
set_property(TARGET libosmscout_binding PROPERTY CXX_STANDARD 11)
install(TARGETS libosmscout_binding
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${OSMSCOUT_BASE_DIR_SOURCE}/Java DESTINATION binding)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Java DESTINATION binding FILES_MATCHING PATTERN "*.java" PATTERN "private" EXCLUDE)
