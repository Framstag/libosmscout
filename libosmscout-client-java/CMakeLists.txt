# libosmscout-client-java — JNI bridge library for Java
#
# Builds:
#   libosmscout_client_java.so  — C++ JNI shared library
#   libosmscoutclientjava.jar   — Java classes with native method declarations

# --- JNI detection ---
find_package(JNI REQUIRED)
find_package(Java COMPONENTS Development REQUIRED)

# --- Java source files (absolute paths for out-of-source builds) ---
set(JAVA_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/java)
set(JAVA_SOURCES
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/AvailableMapEntry.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/CurrentRoadInfo.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/DescriptionEntry.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/FavoriteLocation.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/FavoriteLocationGroup.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/LocationEntry.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/MapDownloadListener.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/MapDownloadManager.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/MapProvider.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/NavigationController.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/NavigationListener.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/NavigationPosition.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/NavigationState.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/ObjectDescription.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/OSMScoutClient.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/OSMScoutClientBuilder.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/RouteCallback.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/RouteEntry.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/RouteInstruction.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/RoutingProfile.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/TrackPoint.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/TurnType.java
  ${JAVA_SOURCES_DIR}/com/framstag/libosmscout/client/Vehicle.java
)

# --- Java compilation + JNI header generation ---
set(JAVA_CLASSES_DIR ${CMAKE_CURRENT_BINARY_DIR}/java/classes)
set(JNI_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/java/headers)

add_custom_command(
  OUTPUT ${JNI_HEADERS_DIR}/com_framstag_libosmscout_client_OSMScoutClient.h
  COMMAND ${CMAKE_COMMAND} -E make_directory ${JAVA_CLASSES_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${JNI_HEADERS_DIR}
  COMMAND ${Java_JAVAC_EXECUTABLE} -h ${JNI_HEADERS_DIR} -d ${JAVA_CLASSES_DIR} ${JAVA_SOURCES}
  DEPENDS ${JAVA_SOURCES}
  COMMENT "Compiling Java sources and generating JNI headers"
)

add_custom_target(java_compile
  DEPENDS ${JNI_HEADERS_DIR}/com_framstag_libosmscout_client_OSMScoutClient.h
)

# --- Jar packaging ---
set(JAR_FILE ${CMAKE_CURRENT_BINARY_DIR}/libosmscoutclientjava.jar)

add_custom_command(
  OUTPUT ${JAR_FILE}
  COMMAND ${Java_JAR_EXECUTABLE} cf ${JAR_FILE} -C ${JAVA_CLASSES_DIR} .
  DEPENDS java_compile
  COMMENT "Packaging Java classes into jar"
)

add_custom_target(java_jar ALL
  DEPENDS ${JAR_FILE}
)

# --- C++ shared library ---
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(CLIENT_JAVA_SOURCES
  src/OSMScoutClient.cpp
)

add_library(osmscout_client_java SHARED
  ${CLIENT_JAVA_SOURCES}
)

target_compile_definitions(osmscout_client_java
  PRIVATE OSMScoutClientJava_EXPORTS
)

target_include_directories(osmscout_client_java
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${JNI_HEADERS_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/java/headers
    ${JNI_INCLUDE_DIRS}
)

target_link_libraries(osmscout_client_java
  PRIVATE
    OSMScout::OSMScout
    OSMScout::Map
    OSMScout::Client
    ${JNI_LIBRARIES}
)

if(TARGET OSMScout::MapCairo)
  target_link_libraries(osmscout_client_java
    PRIVATE
      OSMScout::MapCairo
  )
endif()

# Depend on Java compilation so JNI headers exist before C++ compile
add_dependencies(osmscout_client_java java_compile)

# --- Install rules ---
install(TARGETS osmscout_client_java
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(FILES ${JAR_FILE}
  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
