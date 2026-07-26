## Purpose

GitHub Actions workflow for building JavaScout with CMake — minimal C++ subset plus Maven JavaScout build.

## Requirements

### Requirement: CMake CI workflow

The workflow SHALL build the minimal C++ subset with CMake and then build JavaScout with Maven.

#### Scenario: JDK setup via setup-java
- **WHEN** the job runs
- **THEN** `actions/setup-java@v5` installs JDK 25 temurin with Maven cache

#### Scenario: Minimal C++ deps installed
- **WHEN** the job runs on ubuntu-24.04
- **THEN** only required deps are installed: libxml2, liblzma, libpng, libcairo, libpango, freetype, protobuf

#### Scenario: CMake configured with minimal features
- **WHEN** running `cmake -B build -DOSMSCOUT_BUILD_CLIENT_JAVA=ON` with all other features OFF
- **THEN** configuration succeeds with only core, map, cairo, client, client-java enabled

#### Scenario: Full CMake build
- **WHEN** running `cmake --build build`
- **THEN** all C++ libraries and `libosmscoutclientjava.jar` are built

#### Scenario: CMake install
- **WHEN** running `sudo cmake --install build`
- **THEN** the jar and shared library are installed to system paths

#### Scenario: Jar installed to local Maven repo
- **WHEN** running `mvn install:install-file -Dfile=build/libosmscout-client-java/libosmscoutclientjava.jar`
- **THEN** the jar is available in the local Maven repository

#### Scenario: JavaScout Maven package
- **WHEN** running `mvn -f JavaScout/pom.xml package -q`
- **THEN** JavaScout fat jar is produced at `JavaScout/target/javascout-1.0-SNAPSHOT.jar`

#### Scenario: JavaScout unit tests pass
- **WHEN** running `mvn -f JavaScout/pom.xml test`
- **THEN** all unit tests pass
