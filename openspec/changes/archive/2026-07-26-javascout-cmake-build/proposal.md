## Why

JavaScout currently requires a Meson build for its native JNI layer (`libosmscout-client-java`). The root `CMakeLists.txt` has no reference to the Java client library — no `add_subdirectory`, no build option, no Java/JNI detection. This blocks Android usage where CMake is the standard build tool (via Android NDK's CMake toolchain). It also forces developers who use CMake for C++ libraries to maintain a separate Meson build just for the Java client jar.

Goal: make CMake a first-class build path for JavaScout, enabling Android builds and simplifying the developer workflow.

## What Changes

### 1. New `CMakeLists.txt` for `libosmscout-client-java`

Create a CMake build definition for the JNI C++ library and Java source compilation. This mirrors the existing Meson build:

- **C++ shared library** (`libosmscout_client_java`): compile `src/OSMScoutClient.cpp`, link against `osmscout`, `osmscoutmap`, `osmscoutmapcairo`, `osmscoutclient`, and JNI
- **JNI header generation**: use `javac -h` or CMake's `GenerateJNIHeader` to produce headers from the Java source files
- **Java jar** (`libosmscoutclientjava.jar`): compile all Java sources under `java/` and package into a jar
- **Install rules**: install both the shared library and the jar

### 2. Root `CMakeLists.txt` integration

Add a new option `OSMSCOUT_BUILD_CLIENT_JAVA` (default OFF) that gates the `libosmscout-client-java` subdirectory. Include Java/JNI detection via `FindJNI.cmake`.

### 3. New GitHub Actions workflow: Linux C++/Java build

Single workflow file `build_javascout.yml` with two jobs:

**`cmake` job — CMake flavor:**
- Uses `actions/setup-java@v5` with JDK 25 temurin and Maven cache
- Installs minimal C++ deps (libxml2, liblzma, libpng, libcairo, libpango, freetype)
- CMake configure with `-DOSMSCOUT_BUILD_CLIENT_JAVA=ON` and all other features OFF
- Builds C++ libs, installs them
- Installs jar to local Maven repo via explicit `mvn install:install-file -Dfile=<path>`
- Builds JavaScout with `mvn -f JavaScout/pom.xml package`
- Runs tests with `mvn -f JavaScout/pom.xml test`
- All paths explicit — no `build.sh` or implicit discovery

**`meson` job — Meson flavor:**
- Same JDK setup and Maven cache
- Same minimal C++ deps
- Meson configure with `-Dbuild_java=true` and all other features OFF
- Builds C++ libs
- Installs jar to local Maven repo via explicit `mvn install:install-file -Dfile=<path>`
- Builds JavaScout with `mvn -f JavaScout/pom.xml package`
- Runs tests with `mvn -f JavaScout/pom.xml test`
- All paths explicit — no `build.sh` or implicit discovery

## Capabilities

### New Capabilities

- `client-java-cmake-build`: CMake build definition for `libosmscout-client-java` — shared library + jar, matching Meson feature parity
- `root-cmake-integration`: `OSMSCOUT_BUILD_CLIENT_JAVA` option in root `CMakeLists.txt` with Java/JNI detection
- `javascout-ci-linux-cmake`: GitHub Actions workflow — CMake build + Maven JavaScout
- `javascout-ci-linux-meson`: GitHub Actions workflow — Meson build + Maven JavaScout

### Modified Capabilities

- `javascout-maven-build` (existing): update `build.sh` to also discover CMake-built jar paths; update README to document CMake build path

## Impact

| Area | Impact |
|------|--------|
| `libosmscout-client-java/` | New `CMakeLists.txt` added; existing Meson build unchanged |
| Root `CMakeLists.txt` | New option + `add_subdirectory` + Java/JNI detection |
| `JavaScout/build.sh` | Add CMake build directory to jar discovery paths |
| `JavaScout/README.md` | Document CMake build path alongside Meson |
| `.github/workflows/` | New workflow file `build_javascout_on_ubuntu_24_04.yml` |
| Dependencies | JDK 17+, Apache Maven 3.8+ required when `OSMSCOUT_BUILD_CLIENT_JAVA=ON` |
