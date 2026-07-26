## ADDED Requirements

### Requirement: CMake CI workflow file

The workflow file `.github/workflows/build_javascout_cmake.yml` SHALL build the minimal C++ subset with CMake and then build JavaScout with Maven.

#### Scenario: JDK setup via setup-java
- **WHEN** the job runs
- **THEN** `actions/setup-java@v5` installs JDK 25 temurin with Maven cache

#### Scenario: Minimal C++ deps installed
- **WHEN** the job runs on ubuntu-24.04
- **THEN** only required deps are installed: libxml2, liblzma, libpng, libcairo, libpango, freetype, protobuf

#### Scenario: CMake configured with minimal features
- **WHEN** running `cmake -B build -DOSMSCOUT_BUILD_CLIENT_JAVA=ON -DOSMSCOUT_BUILD_IMPORT=OFF -DOSMSCOUT_BUILD_GPX=OFF -DOSMSCOUT_BUILD_MAP_AGG=OFF -DOSMSCOUT_BUILD_MAP_CAIRO=ON -DOSMSCOUT_BUILD_MAP_OPENGL=OFF -DOSMSCOUT_BUILD_MAP_QT=OFF -DOSMSCOUT_BUILD_MAP_SVG=OFF -DOSMSCOUT_BUILD_CLIENT_QT=OFF -DOSMSCOUT_BUILD_DEMOS=OFF -DOSMSCOUT_BUILD_TOOL_IMPORT=OFF -DOSMSCOUT_BUILD_TOOL_DUMPDATA=OFF -DOSMSCOUT_BUILD_TOOL_MCPSERVER=OFF -DOSMSCOUT_BUILD_TOOL_PUBLICTRANSPORTMAP=OFF -DOSMSCOUT_BUILD_TOOL_OSMSCOUT2=OFF -DOSMSCOUT_BUILD_TOOL_OSMSCOUTOPENGL=OFF -DOSMSCOUT_BUILD_TOOL_STYLEEDITOR=OFF -DOSMSCOUT_BUILD_TESTS=OFF`
- **THEN** configuration succeeds with only core, map, cairo, client, client-java enabled

#### Scenario: Full CMake build
- **WHEN** running `cmake --build build`
- **THEN** all C++ libraries and `libosmscoutclientjava.jar` are built

#### Scenario: CMake install
- **WHEN** running `sudo cmake --install build`
- **THEN** the jar and shared library are installed to system paths

#### Scenario: Jar installed to local Maven repo
- **WHEN** running `mvn install:install-file -Dfile=build/libosmscout-client-java/libosmscoutclientjava.jar -DgroupId=net.sf.libosmscout -DartifactId=libosmscout-client-java -Dversion=1.0-SNAPSHOT -Dpackaging=jar -q`
- **THEN** the jar is available in the local Maven repository

#### Scenario: JavaScout Maven package
- **WHEN** running `mvn -f JavaScout/pom.xml package -q`
- **THEN** JavaScout fat jar is produced at `JavaScout/target/javascout-1.0-SNAPSHOT.jar`

#### Scenario: JavaScout unit tests pass
- **WHEN** running `mvn -f JavaScout/pom.xml test`
- **THEN** all unit tests pass
