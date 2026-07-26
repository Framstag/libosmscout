## Purpose

Build tooling for JavaScout — jar discovery in build.sh and build documentation in README.

## Requirements

### Requirement: Jar discovery in build.sh

`build.sh` SHALL search `debug`, `build`, `build-meson`, `build-release`, `build-cmake`, `build-release-cmake` for the jar, checking both Meson and CMake output layouts.

#### Scenario: CMake build directory discovered
- **WHEN** `build.sh` runs without arguments and a CMake build exists at `build-cmake`
- **THEN** the jar at `build-cmake/libosmscout-client-java/libosmscoutclientjava.jar` is found and installed

#### Scenario: Meson build still works
- **WHEN** `build.sh` runs without arguments and a Meson build exists at `build`
- **THEN** the jar at `build/libosmscout-client-java/java/libosmscoutclientjava.jar` is found and installed

### Requirement: README documents CMake build path

`JavaScout/README.md` SHALL document both Meson and CMake build paths.

#### Scenario: CMake build instructions present
- **WHEN** reading `JavaScout/README.md`
- **THEN** it includes a CMake build section showing `cmake -B build -DOSMSCOUT_BUILD_CLIENT_JAVA=ON && cmake --build build`

#### Scenario: Meson instructions preserved
- **WHEN** reading `JavaScout/README.md`
- **THEN** the existing Meson build instructions are still present
