if(OSMSCOUT_BUILD_EXTERN_MATLAB)
  if(NOT MATLAB_FOUND)
    message(SEND_ERROR "MATLAB is required for MATLAB mex function")
  endif()

  matlab_add_mex (NAME "osmscoutmx" SRC "src/matlab/libosmscoutmx.cpp" "src/matlab/libosmscoutmx.h")

  if(MSVC)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/osmscoutmxd.def "LIBRARY osmscoutmxd.${Matlab_MEX_EXTENSION}\n\nEXPORTS\n    mexFunction\n")
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/osmscoutmx.def "LIBRARY osmscoutmx.${Matlab_MEX_EXTENSION}\n\nEXPORTS\n    mexFunction\n")
    set_target_properties(osmscoutmx PROPERTIES LINK_FLAGS -DEF:"${CMAKE_CURRENT_BINARY_DIR}/osmscoutmx.def")
    set_target_properties(osmscoutmx PROPERTIES LINK_FLAGS_DEBUG -DEF:"${CMAKE_CURRENT_BINARY_DIR}/osmscoutmxd.def")
	create_win32_dll_resource(osmscoutmx "osmscoutmx")
  endif()

  target_link_libraries(osmscoutmx OSMScout::OSMScout)
  target_compile_definitions(osmscoutmx PRIVATE -DOSMSCOUT_MATLAB_EXPORT_SYMBOLS -DMATLAB_MEX_FILE)

  if(${OSMSCOUT_BUILD_MAP_CAIRO})
    target_link_libraries(osmscoutmx OSMScout::Map OSMScout::MapCairo)
    target_compile_definitions(osmscoutmx PRIVATE -DOSMSCOUT_MAP_CAIRO)
  endif()

  set_target_properties(osmscoutmx PROPERTIES VERSION ${OSMSCOUT_LIBRARY_VERSION} SOVERSION ${OSMSCOUT_LIBRARY_VERSION} FOLDER "Extern")

  install(TARGETS osmscoutmx
    RUNTIME DESTINATION bin/matlab
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
endif()
