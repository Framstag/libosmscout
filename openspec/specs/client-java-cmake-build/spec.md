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

The CMake build SHALL generate JNI native headers from the Java source files before compiling the C++ library. The generated header directory SHALL contain the headers of the Java sources present when the generation ran and no header of a source that is no longer present, so that a removed or renamed native method cannot leave a stale header for the C++ build to include.

#### Scenario: Headers generated before C++ compile
- **WHEN** running `cmake --build`
- **THEN** `javac -h` runs on all Java sources under `java/` and headers are written to a known include directory

#### Scenario: All Java sources produce headers
- **WHEN** checking the generated header directory
- **THEN** one header per class that declares a native method is present — for the current sources
  `MapDownloadManager`, `NavigationController`, `OSMScoutClientBuilder` and `OSMScoutClient` — because
  `javac -h` emits a header only for a class that declares one

#### Scenario: A header of a removed source is not kept
- **GIVEN** a build tree whose JNI header directory holds a header that corresponds to no Java source of the current tree
- **WHEN** the JNI header generation runs again
- **THEN** that header is gone afterwards and the directory holds the headers of the current sources

### Requirement: Java jar target

The CMake build SHALL produce `libosmscoutclientjava.jar` from all Java sources under `java/`. The build SHALL regenerate the jar whenever a Java source it packages or the generated JNI header changes, so that the jar in the build tree and the installed jar always correspond to the sources present when the build ran. A jar SHALL NOT stay at its previous contents after such a change, and regenerating it SHALL NOT require deleting the jar by hand.

#### Scenario: Jar is packaged
- **WHEN** running `cmake --build`
- **THEN** a jar file `libosmscoutclientjava.jar` is created containing all compiled `.class` files

#### Scenario: Install rules
- **WHEN** running `cmake --install`
- **THEN** the jar is installed to `${CMAKE_INSTALL_LIBDIR}` alongside the shared library

#### Scenario: Jar follows a changed Java source
- **GIVEN** a build tree in which the jar has already been produced
- **WHEN** a Java source under `java/` is modified and the build is run again
- **THEN** the jar is repackaged in that run, so it is newer than the jar of the previous build and contains the class of the modified source
- **AND** no manual deletion of the jar is needed to obtain it

#### Scenario: Jar follows the generated JNI header
- **GIVEN** a build tree in which the jar has already been produced
- **WHEN** the generated JNI header changes because a native method declaration changed
- **THEN** the jar is repackaged in the same build run

#### Scenario: A jar that is deleted is rebuilt
- **GIVEN** a build tree whose jar was removed from the build directory
- **WHEN** the build is run again
- **THEN** the jar is produced again without any other source change

### Requirement: Meson parity

The CMake build and the Meson build SHALL compile and package the same set of Java source files under `java/`. Every `.java` file the CMake build compiles SHALL also be listed by the Meson build, in both its JNI header inputs and its jar inputs, and every `.java` file the Meson build lists SHALL also be compiled by the CMake build. A jar produced by one build system SHALL NOT contain fewer classes than the jar produced by the other for the same sources.

#### Scenario: Source list matches Meson
- **WHEN** comparing source lists
- **THEN** every `.java` file listed in `java/meson.build` is also compiled by the CMake build

#### Scenario: Meson packages every source CMake compiles
- **GIVEN** the Java sources under `java/`
- **WHEN** the jar of each build system is built and its entries are listed
- **THEN** each jar contains a class for every `.java` file under `java/`
- **AND** the two entry sets are equal

#### Scenario: A source missing from one list is observable
- **GIVEN** a Java source under `java/` that one build system's list does not name
- **WHEN** that build system's jar entries are compared with the source directory and with the other build system's jar
- **THEN** the missing class is reported by the comparison instead of the jar being silently smaller

#### Scenario: Adding a Java source to the client
- **GIVEN** a new Java source added under `java/`
- **WHEN** both build systems package the client jar
- **THEN** both lists name the new source and both jars contain its class
