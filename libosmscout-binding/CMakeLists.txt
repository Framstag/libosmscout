if(OSMSCOUT_BUILD_BINDING_JAVA)
  if(NOT SWIG_FOUND)
    message(SEND_ERROR "swig is required for java binding")
  endif()
  if(NOT JNI_FOUND)
    message(SEND_ERROR "A JDK is required for java binding")
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
endif()

if(OSMSCOUT_BUILD_BINDING_MATLAB)
  if(NOT MATLAB_FOUND)
    message(SEND_ERROR "MATLAB is required for MATLAB binding")
  endif()

  matlab_add_mex (NAME "libosmscoutmx" SRC "src/matlab/libosmscoutmx.cpp" "src/matlab/libosmscoutmx.h")
  if(MSVC)
    file(WRITE ${CMAKE_BINARY_DIR}/libosmscoutmxd.def "LIBRARY libosmscoutmxd.${Matlab_MEX_EXTENSION}\n\nEXPORTS\n    mexFunction\n")
    file(WRITE ${CMAKE_BINARY_DIR}/libosmscoutmx.def "LIBRARY libosmscoutmx.${Matlab_MEX_EXTENSION}\n\nEXPORTS\n    mexFunction\n")
    set_target_properties(libosmscoutmx PROPERTIES LINK_FLAGS -DEF:"${CMAKE_BINARY_DIR}/libosmscoutmx.def")
    set_target_properties(libosmscoutmx PROPERTIES LINK_FLAGS_DEBUG -DEF:"${CMAKE_BINARY_DIR}/libosmscoutmxd.def")
  endif()
  target_include_directories(libosmscoutmx PRIVATE 
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
  target_link_libraries(libosmscoutmx libosmscout)
  target_compile_definitions(libosmscoutmx PRIVATE -DOSMSCOUT_MATLAB_EXPORT_SYMBOLS -DMATLAB_MEX_FILE)
  if(${OSMSCOUT_BUILD_IMPORT})
    target_include_directories(libosmscoutmx PRIVATE ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-import/include)
    target_link_libraries(libosmscoutmx libosmscout_import)
    target_compile_definitions(libosmscoutmx PRIVATE -DOSMSCOUT_IMPORT)
  endif()
  if(${OSMSCOUT_BUILD_MAP_CAIRO})
    target_include_directories(libosmscoutmx PRIVATE
      ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
      ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-cairo/include
      ${CAIRO_INCLUDE_DIRS})
    target_link_libraries(libosmscoutmx
      libosmscout_map
      libosmscout_map_cairo
      ${CAIRO_LIBRARIES})
    target_compile_definitions(libosmscoutmx PRIVATE -DOSMSCOUT_MAP_CAIRO)
  endif()
  set_property(TARGET libosmscoutmx PROPERTY CXX_STANDARD 11)
  install(TARGETS libosmscoutmx
    RUNTIME DESTINATION bin/matlab
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
endif()
