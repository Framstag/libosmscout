## Context

JavaScout is a JavaFX desktop app that depends on `libosmscout-client-java` — a JNI bridge library. The JNI library has two parts:

1. **C++ shared library** (`libosmscout_client_java.so`): wraps core libosmscout C++ APIs via JNI
2. **Java jar** (`libosmscoutclientjava.jar`): Java classes that call native methods

Currently only Meson builds `libosmscout-client-java`. The root `CMakeLists.txt` has no Java/JNI support.

## Goals / Non-Goals

**Goals:**
- `libosmscout-client-java` builds with CMake (shared lib + jar) on Linux
- Root `CMakeLists.txt` has `OSMSCOUT_BUILD_CLIENT_JAVA` option gating the subdirectory
- Two CI workflows: CMake-based and Meson-based, both producing the jar and building JavaScout with Maven
- `JavaScout/build.sh` discovers jars from CMake build dirs too
- `JavaScout/README.md` documents both build paths

**Non-Goals:**
- Not replacing the Meson build — both coexist
- Not porting `libosmscout-binding` (SWIG Java bindings) to CMake — that's a separate concern
- Not porting `libosmscout-map-binding` to CMake
- Not adding Windows or macOS CMake+Java CI (future work)
- Not adding Android CMake support (future change)

## Decisions

### Decision 1: CMake `add_jar` via custom target vs FindJNI

**Chosen: Custom CMake functions using `FindJNI` + `javac` + `jar` commands**

CMake doesn't ship a built-in `add_jar`. Options:
- **Use `FindJNI.cmake`** (ships with CMake) to locate JDK, JNI headers, and `tools.jar`
- **Use a custom `add_jar` macro** (many projects roll their own) that calls `javac` and `jar` via `add_custom_target`
- **Use the Gradle/Maven wrapper** — too heavy for a C++ build system

Rationale: `FindJNI` is standard and reliable. A thin `add_jar` wrapper keeps the CMakeLists.txt clean. The Meson build already does the same thing internally (calls `javac` + `jar` via the `java` module).

### Decision 2: JNI header generation

**Chosen: `javac -h <dir>` during Java compilation**

CMake has no native JNI header generation. The `javac -h` flag (Java 8+) outputs native headers alongside `.class` files. This matches what Meson's `native_headers()` does internally.

The C++ library target depends on the Java compilation target, ensuring headers exist before compilation.

### Decision 3: CI workflow structure

**Chosen: Single workflow file `build_javascout.yml` with two jobs (`cmake` + `meson`)**

Each builds only the minimal subset: core, map, cairo, client, client-java. No import, no Qt, no OpenGL, no AGG, no tests, no demos, no tools. This keeps CI fast and focused on JavaScout.

Both use `actions/setup-java@v5` with JDK 25 temurin and Maven caching.

### Decision 4: Jar discovery in `build.sh`

**Chosen: `debug` first, then other build dirs**

Search order: `debug`, `build`, `build-meson`, `build-release`, `build-cmake`, `build-release-cmake`

`debug` is first because it's the most common local development build directory.

### Decision 5: CI uses explicit Maven commands, not build.sh

**Chosen: CI workflows call `mvn install:install-file` + `mvn package` + `mvn test` directly with explicit paths**

`build.sh` is a developer convenience script. CI pipelines must be explicit:
- Jar path is constructed from the known build directory layout
- `mvn install:install-file -Dfile=<explicit-path> -DgroupId=net.sf.libosmscout -DartifactId=libosmscout-client-java -Dversion=1.0-SNAPSHOT -Dpackaging=jar`
- `mvn -f JavaScout/pom.xml package`
- `mvn -f JavaScout/pom.xml test`

This avoids brittle path discovery and makes failures immediately clear.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| CMake `FindJNI` may not find JDK on all platforms | Document `JAVA_HOME` requirement; CI sets it explicitly |
| JNI header paths differ between Meson and CMake | Use `javac -h` to a known dir; C++ `target_include_directories` points there |
| Dual build system maintenance burden | Keep Meson as source of truth; CMake mirrors its structure. CI validates both |
| `add_jar` custom macro fragility | Keep it simple — inline `javac` + `jar` commands, not a complex macro |
