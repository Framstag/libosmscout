## 1. CMakeLists.txt for libosmscout-client-java

- [x] 1.1 Create `libosmscout-client-java/CMakeLists.txt` with shared library target `osmscout_client_java` from `src/OSMScoutClient.cpp` (spec: `client-java-cmake-build`)
- [x] 1.2 Add `FindJNI` detection and JNI include dirs/libraries to the target (spec: `client-java-cmake-build`)
- [x] 1.3 Add Java compilation via `add_custom_target` calling `javac` on all sources under `java/` (spec: `client-java-cmake-build`)
- [x] 1.4 Add JNI header generation via `javac -h` before C++ compile (spec: `client-java-cmake-build`)
- [x] 1.5 Add jar packaging via `add_custom_target` calling `jar` on compiled classes (spec: `client-java-cmake-build`)
- [x] 1.6 Add install rules for both the shared library and the jar (spec: `client-java-cmake-build`)
## 2. Root CMakeLists.txt integration

- [x] 2.1 Add `find_package(JNI)` and `OSMSCOUT_BUILD_CLIENT_JAVA` option (default OFF) in root `CMakeLists.txt` (spec: `root-cmake-integration`)
- [x] 2.2 Add `add_subdirectory(libosmscout-client-java)` gated by the option (spec: `root-cmake-integration`)
- [x] 2.3 Add build summary line for `OSMSCOUT_BUILD_CLIENT_JAVA` (spec: `root-cmake-integration`)

## 3. JavaScout build.sh and README updates

- [x] 3.1 Reorder jar search in `JavaScout/build.sh`: `debug` first, then `build`, `build-meson`, `build-release`, `build-cmake`, `build-release-cmake` (spec: `javascout-maven-build`)
- [x] 3.2 Add CMake build instructions section to `JavaScout/README.md` (spec: `javascout-maven-build`)

## 4. GitHub Actions workflow

- [x] 4.1 Create `.github/workflows/build_javascout.yml` with `cmake` and `meson` jobs, both using `actions/setup-java@v5` (JDK 25 temurin, Maven cache), minimal C++ deps, all non-essential features OFF, then explicit `mvn install:install-file`, `mvn -f JavaScout/pom.xml package`, `mvn -f JavaScout/pom.xml test` — no `build.sh` (specs: `javascout-ci-linux-cmake`, `javascout-ci-linux-meson`)
