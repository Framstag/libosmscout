## Purpose

CMake build definition for `libosmscout-client-java` — produces the JNI shared library and Java jar, matching Meson feature parity.

## Requirements

### Requirement: CMake library target for `osmscout_client_java`

The CMake build SHALL produce a shared library `libosmscout_client_java` from `src/OSMScoutClient.cpp`.

#### Scenario: Shared library builds
- **WHEN** CMake configures with `-DOSMSCOUT_BUILD_CLIENT_JAVA=ON`
- **THEN** target `osmscout_client_java` exists and links against `osmscout`, `osmscoutmap`, `osmscoutmapcairo`, `osmscoutclient`, and JNI libraries

#### Scenario: Export symbols defined
- **WHEN** building as shared library
- **THEN** `OSMScoutClientJava_EXPORTS` define is set and visibility flags match Meson behavior

### Requirement: JNI header generation

The CMake build SHALL generate JNI native headers from the Java source files before compiling the C++ library.

#### Scenario: Headers generated before C++ compile
- **WHEN** running `cmake --build`
- **THEN** `javac -h` runs on all Java sources under `java/` and headers are written to a known include directory

#### Scenario: All Java sources produce headers
- **WHEN** checking the generated header directory
- **THEN** headers exist for all classes listed in `java/meson.build`'s `native_headers()` call

### Requirement: Java jar target

The CMake build SHALL produce `libosmscoutclientjava.jar` from all Java sources under `java/`.

#### Scenario: Jar is packaged
- **WHEN** running `cmake --build`
- **THEN** a jar file `libosmscoutclientjava.jar` is created containing all compiled `.class` files

#### Scenario: Install rules
- **WHEN** running `cmake --install`
- **THEN** the jar is installed to `${CMAKE_INSTALL_LIBDIR}` alongside the shared library

### Requirement: Meson parity

The CMake build SHALL support the same set of Java source files as the existing Meson build.

#### Scenario: Source list matches Meson
- **WHEN** comparing source lists
- **THEN** every `.java` file listed in `java/meson.build` is also compiled by the CMake build
