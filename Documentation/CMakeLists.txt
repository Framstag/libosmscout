if(NOT DOXYGEN_FOUND)
  message(SEND_ERROR "Doxygen is required to build the documentation.")
endif()

if(DOXYGEN_DOT_FOUND)
  set(OSMSCOUT_HAVE_DOT "YES")
  set(OSMSCOUT_DOT_PATH "${DOXYGEN_DOT_EXECUTABLE}")
else()
  set(OSMSCOUT_HAVE_DOT "NO")
  set(OSMSCOUT_DOT_PATH "")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.cmake ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)

if (WIN32 OR APPLE)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION doc)
elseif (UNIX)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION "share/osmscout/docs")
endif ()
