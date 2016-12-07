if(OSMSCOUT_BUILD_EXTERN_MATLAB)
  if(NOT MATLAB_FOUND)
    message(SEND_ERROR "MATLAB is required for MATLAB mex function")
  endif()

  matlab_add_mex (NAME "osmscoutmx" SRC "src/matlab/libosmscoutmx.cpp" "src/matlab/libosmscoutmx.h")
  if(MSVC)
    file(WRITE ${CMAKE_BINARY_DIR}/osmscoutmxd.def "LIBRARY osmscoutmxd.${Matlab_MEX_EXTENSION}\n\nEXPORTS\n    mexFunction\n")
    file(WRITE ${CMAKE_BINARY_DIR}/osmscoutmx.def "LIBRARY osmscoutmx.${Matlab_MEX_EXTENSION}\n\nEXPORTS\n    mexFunction\n")
    set_target_properties(osmscoutmx PROPERTIES LINK_FLAGS -DEF:"${CMAKE_BINARY_DIR}/osmscoutmx.def")
    set_target_properties(osmscoutmx PROPERTIES LINK_FLAGS_DEBUG -DEF:"${CMAKE_BINARY_DIR}/osmscoutmxd.def")
  endif()
  target_include_directories(osmscoutmx PRIVATE 
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout/include)
  target_link_libraries(osmscoutmx osmscout)
  target_compile_definitions(osmscoutmx PRIVATE -DOSMSCOUT_MATLAB_EXPORT_SYMBOLS -DMATLAB_MEX_FILE)
  if(${OSMSCOUT_BUILD_MAP_CAIRO})
    target_include_directories(osmscoutmx PRIVATE
      ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map/include
      ${OSMSCOUT_BASE_DIR_SOURCE}/libosmscout-map-cairo/include
      ${CAIRO_INCLUDE_DIRS})
    target_link_libraries(osmscoutmx
      osmscout_map
      osmscout_map_cairo
      ${CAIRO_LIBRARIES})
    target_compile_definitions(osmscoutmx PRIVATE -DOSMSCOUT_MAP_CAIRO)
  endif()
  set_property(TARGET osmscoutmx PROPERTY CXX_STANDARD 11)
  install(TARGETS osmscoutmx
    RUNTIME DESTINATION bin/matlab
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
endif()
