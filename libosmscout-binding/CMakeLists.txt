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
  # See https://github.com/swig/swig/issues/568
  set(CMAKE_SWIG_FLAGS -c++ -package net.sf.libosmscout.osmscout -features autodoc=1 -DSWIGWORDSIZE64 -I${CMAKE_CURRENT_SOURCE_DIR}/src/swig -I${CMAKE_CURRENT_SOURCE_DIR}/../libosmscout/include -outdir ${CMAKE_SWIG_OUTDIR})
  if(MSVC)
    set(CMAKE_SWIG_FLAGS ${CMAKE_SWIG_FLAGS} -Fmicrosoft)
  endif()
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/swig/osmscout/libosmscout.i PROPERTIES CPLUSPLUS ON)
  swig_add_library(osmscout_binding_java
    LANGUAGE Java
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/swig/osmscout/libosmscout.i)
  swig_link_libraries(osmscout_binding_java OSMScout ${JNI_LIBRARIES})
  target_include_directories(osmscout_binding_java PRIVATE
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${JNI_INCLUDE_DIRS}
    ${JAVA_INCLUDE_PATH})
  target_link_libraries(osmscout_binding_java
    ${OSMSCOUT_TARGET}
    ${JNI_LIBRARIES})
  set_property(TARGET osmscout_binding_java PROPERTY CXX_STANDARD 14)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

  install(TARGETS osmscout_binding_java
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)
  install(DIRECTORY ${OSMSCOUT_BASE_DIR_SOURCE}/Java DESTINATION binding)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Java DESTINATION binding FILES_MATCHING PATTERN "*.java" PATTERN "private" EXCLUDE)
endif()

if(OSMSCOUT_BUILD_BINDING_CSHARP)
  if(NOT SWIG_FOUND)
    message(SEND_ERROR "swig is required for c# binding")
  endif()

  include(${SWIG_USE_FILE})
  set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR}/CSharp)
  file(MAKE_DIRECTORY ${CMAKE_SWIG_OUTDIR})
  set(CMAKE_SWIG_FLAGS -c++ -features autodoc=1 -I${CMAKE_CURRENT_SOURCE_DIR}/src/swig -I${CMAKE_CURRENT_SOURCE_DIR}/../libosmscout/include -outdir ${CMAKE_SWIG_OUTDIR})
  if(MSVC)
    set(CMAKE_SWIG_FLAGS ${CMAKE_SWIG_FLAGS} -Fmicrosoft)
  endif()
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/swig/osmscout/libosmscout.i PROPERTIES CPLUSPLUS ON)
  swig_add_module(osmscout_binding_csharp csharp ${CMAKE_CURRENT_SOURCE_DIR}/src/swig/osmscout/libosmscout.i)
  swig_link_libraries(osmscout_binding_csharp OSMScout)
  target_include_directories(osmscout_binding_csharp PRIVATE
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include)
  target_link_libraries(osmscout_binding_csharp
          ${OSMSCOUT_TARGET})
  set_property(TARGET osmscout_binding_csharp PROPERTY CXX_STANDARD 14)

  install(TARGETS osmscout_binding_csharp
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/CSharp DESTINATION binding FILES_MATCHING PATTERN "*.cs" PATTERN "private" EXCLUDE)
endif()
