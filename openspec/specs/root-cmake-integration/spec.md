## Purpose

Integration of the Java client library build into the root CMake build system — option, JNI detection, and build summary.

## Requirements

### Requirement: `OSMSCOUT_BUILD_CLIENT_JAVA` option

The root `CMakeLists.txt` SHALL define option `OSMSCOUT_BUILD_CLIENT_JAVA` that defaults to ON when JNI is found, OFF otherwise.

#### Scenario: Option defaults ON with JDK
- **WHEN** running `cmake -LAH` with a JDK installed
- **THEN** `OSMSCOUT_BUILD_CLIENT_JAVA` appears with default ON

#### Scenario: Option defaults OFF without JDK
- **WHEN** running `cmake -LAH` without a JDK
- **THEN** `OSMSCOUT_BUILD_CLIENT_JAVA` appears with default OFF

#### Scenario: Subdirectory added when ON
- **WHEN** `OSMSCOUT_BUILD_CLIENT_JAVA=ON` and Java/JNI are found
- **THEN** `add_subdirectory(libosmscout-client-java)` is called

#### Scenario: Subdirectory skipped when OFF
- **WHEN** `OSMSCOUT_BUILD_CLIENT_JAVA=OFF`
- **THEN** `libosmscout-client-java` is not added as a subdirectory

### Requirement: Java/JNI detection

The root `CMakeLists.txt` SHALL use `FindJNI.cmake` to detect JDK and JNI headers.

#### Scenario: JDK found
- **WHEN** CMake configures with `OSMSCOUT_BUILD_CLIENT_JAVA=ON`
- **THEN** `JNI_FOUND` is true and `JNI_INCLUDE_DIRS` and `JNI_LIBRARIES` are set

#### Scenario: JDK not found
- **WHEN** CMake configures with `OSMSCOUT_BUILD_CLIENT_JAVA=ON` but no JDK is installed
- **THEN** a fatal error message tells the user to install a JDK

### Requirement: Build summary

The root `CMakeLists.txt` SHALL print `OSMSCOUT_BUILD_CLIENT_JAVA` status in the build configuration summary.

#### Scenario: Summary line printed
- **WHEN** CMake finishes configuring
- **THEN** output includes `- Java client library: ON` or `OFF`
