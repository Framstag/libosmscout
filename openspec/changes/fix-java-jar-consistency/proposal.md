# Proposal

## Why

The Java client library ships two artifacts — the JNI shared library and `libosmscoutclientjava.jar` — and the jar is produced by both the CMake and the Meson build, which are kept at feature parity. Today the jar can be silently wrong in two ways. A CMake build keeps serving a jar from before a Java source was touched, so a build or an `install` can ship classes that do not match the sources, and a Java consumer such as JavaScout then fails to compile against it. The Meson build packages a smaller source set than CMake, so the jar of one build system lacks a class the other one has. Both defects are invisible in the build log and surface only in a downstream compile or at runtime, which is why they are worth closing together: they are the same contract — the jar is built from the Java sources — violated once per build system.

Verifying the client against the library it ships with uncovered a third defect in the same area: the JNI bridge read the magnification of `render` and `projectToPixel` as a floating-point scale factor while the Java declarations (and every caller) pass an integer level. The value the bridge read was therefore garbage (~1e288), the level derived from it (957) indexed past the fixed cell-size table, and the client aborted the process in a build with asserts — the sanitizer-free Meson build the test recipe uses. A release build (asserts compiled out) instead read past the table silently, which is why this was not noticed earlier.

## What Changes

- The CMake build rebuilds the jar whenever the Java sources it packages or the generated JNI header change; the jar no longer stays at its previous contents after a Java source was edited.
- Both build systems package the complete set of Java sources under `java/`: the Meson jar includes `CurrentRoadInfo.java`, which CMake already compiles.
- A jar produced by either build system contains the same classes, so a consumer of the jar can rely on the artifact rather than on the build system that produced it.
- A repeated build after touching a Java source produces a jar with a new timestamp and the changed class; no manual deletion of the jar is required to get it.
- No Java API, native interface, or installation directory changes.
- The magnification of `render`, `renderWithRouteAndPois`, `renderWithRoute` and `projectToPixel` is the **scale factor** (2^zoom level) the JNI bridge and NaviVeylin's own binding use; fractional values are supported, a scale below 1 or above the level range of the cell-size table is refused with a warning instead of aborting, and the cell-size lookup itself never reads past that table — in a build with and without asserts alike. JavaScout and the tests pass the scale derived from their integer zoom level.

## Capabilities

### New Capabilities

- `client-java-jni-binding`: the agreement between the Java `native` declarations and the JNI functions, and the magnification crossing that boundary.
- `magnification-levels`: the valid magnification level range and the bounds-safe cell-size lookup.

### Modified Capabilities

- `client-java-cmake-build`: the jar target rebuilds when the sources it packages change (instead of only when the target is invoked), and the source-list parity requirement becomes bidirectional and complete — neither build system may package a smaller Java source set than the other.

## Impact

- `libosmscout-client-java/CMakeLists.txt` — the jar packaging rule and the target that drives it.
- `libosmscout-client-java/java/meson.build` — both Java source lists (the JNI header inputs and the jar inputs).
- `libosmscout-client-java/meson.build`, `libosmscout-client-java/CMakeLists.txt` — no change to the shared library, JNI header generation, or the install rules.
- Consumers of the jar: `JavaScout` (compiles against the installed jar), `Android`/`Apple` builds that copy the jar, and the C++ JNI library that must stay in step with the classes.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the parameter types of `render`, `renderWithRouteAndPois` and `projectToPixel` and the scale validation they gained.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` — the magnification parameter and its documentation (scale factor).
- `JavaScout/src/main/java/com/framstag/libosmscout/MapRenderer.java` and the client tests — pass the scale derived from their zoom level.
- `scripts/check-jni-signatures.sh` (new) — the host-side comparison of the Java declarations with the JNI functions, registered as a test in `Tests/CMakeLists.txt` and `Tests/meson.build`.
- `libosmscout/src/osmscout/util/TileId.cpp` — the bounds-safe cell-size lookup.
- Measured while preparing this change: with the mismatch, the bridge derived level 957 from the garbage value and the JVM aborted (exit 134) in the asserts-enabled Meson build; after the fix the JavaScout suite runs (227 tests, 0 failures, 0 errors, 18 skipped, with `SearchReproTest` excluded — that class has its own pre-existing `bad_alloc` abort) and the signature check covers 55 declarations.
- No public C++ or Java API changes; no file format changes.
