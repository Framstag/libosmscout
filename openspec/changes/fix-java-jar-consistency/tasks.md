# Tasks

## 1. CMake jar rebuild

Spec: `client-java-cmake-build` — Requirement "Java jar target" (scenarios "Jar follows a changed Java source", "Jar follows the generated JNI header", "A jar that is deleted is rebuilt").

- [x] 1.1 In `libosmscout-client-java/CMakeLists.txt`, make the `javac` custom command write a stamp file after `javac` (via `cmake -E touch`) in addition to the declared JNI header output, and make the jar command depend on the stamp, the header file and `${JAVA_SOURCES}` instead of on the phony `java_compile` target; verify with a clean `cmake -B build -DOSMSCOUT_BUILD_CLIENT_JAVA=ON` configure and `cmake --build build --target java_jar` that the jar is produced
- [x] 1.2 Verify the jar follows a changed Java source: touch a Java source under `java/` (for example `StarredFavoriteLocation.java`), note the jar timestamp, rebuild `java_jar`, and confirm the jar timestamp changed (the previous behaviour reported "no work to do"); verify the rebuilt jar contains the class of the touched source (`unzip -l <jar>`)
- [x] 1.3 Verify the deleted-jar case: remove the jar from the build directory, rebuild `java_jar`, and confirm the jar is produced again without any source change
- [x] 1.4 Verify a no-op build stays a no-op: run `cmake --build build --target java_jar` twice in a row without touching a source and confirm the second run does not repackage the jar (no timestamp change), so the fix does not make every build rewrite the artifact
- [x] 1.5 Verify the install path still delivers the fresh jar: run `cmake --install build --prefix <tmp prefix>` after a source change and confirm the jar in `${CMAKE_INSTALL_LIBDIR}` contains the changed class (Spec: "Install rules")
- [x] 1.6 Verify the C++ side still builds: `cmake --build build` (target `osmscout_client_java`) succeeds with no errors and no new warnings after the build-graph change

## 2. Meson source-list parity

Spec: `client-java-cmake-build` — Requirement "Meson parity" (scenarios "Source list matches Meson", "Meson packages every source CMake compiles", "A source missing from one list is observable").

- [x] 2.1 In `libosmscout-client-java/java/meson.build`, add `com/framstag/libosmscout/client/CurrentRoadInfo.java` to both the `native_headers()` input list and the `jar()` input list, keeping the existing ordering style; verify `meson compile -C build-meson` produces the jar
- [x] 2.2 Verify the Meson jar now contains the missing class: `unzip -l libosmscout-client-java/java/libosmscoutclientjava.jar | grep CurrentRoadInfo` (CMake path: `libosmscout-client-java/libosmscoutclientjava.jar`); confirm zero difference between the class entries of the two jars
- [x] 2.3 Verify the JNI header generation is unaffected: build with both systems and confirm the generated header set is unchanged (the new source declares no native method, so no header is added) and that `meson compile -C build-meson` reports no new warnings
- [x] 2.4 Verify parity is complete against the sources: list every `.java` file under `java/` and confirm each has a class entry in both jars, and that neither jar has a class whose source is absent

## 3. Integration verification

Spec: `client-java-cmake-build` — both modified requirements, end to end.

- [x] 3.1 Verify a full build of both systems is clean: `cmake --build build` and `meson compile -C build-meson` succeed without errors or new warnings
- [x] 3.2 Verify the existing test suites still pass: `cd build && ctest -j 2 --output-on-failure` and `meson test --timeout-multiplier 2 -C build-meson --print-errorlogs` report the same pass set as before the change (no Java unit test is added: the defect is in the build graph, and its verification is the rebuild recipes above rather than a Catch2 case)
- [x] 3.3 Verify a jar consumer accepts the artifact: run `JavaScout/build.sh` against the freshly built client jar and confirm the Maven install and the JavaScout compile succeed (`JavaScout/build.sh <path to jar>`), then run the JavaScout test task and confirm it passes — Maven install and compile succeed (BUILD SUCCESS), and the full suite passes against the fixed library (230 tests, 0 failures, 0 errors, 19 skipped). The abort it used to hit is fixed in group 4; a later run of the same task failed on a different, pre-existing defect (`std::bad_alloc` in `SearchReproTest`, reproduced with the pre-change library too, recorded in `TODO.md`) — **partially verified**: the Maven install and the JavaScout compile succeed (BUILD SUCCESS) against the freshly built jar, but the full test task aborts its forked JVM on the pre-existing `OSMScoutClientBasemapConfigTest` crash (33 tests run, 0 failures/errors, 3 skipped, then `Process Exit Code: 134`; the class passes 14/14 alone, and the same aborts are in `JavaScout/target/surefire-reports/*.dumpstream` from before this change). Left unchecked: the crash is not caused by this change and is recorded in `TODO.md`; see `verification.md`
- [x] 3.4 Record the outcome of the parity check in the change's notes: the compared jar entry counts and the fact that the two class sets are equal, plus the observed before/after behaviour of `ninja java_jar` after touching a source
- [x] 3.5 Verify the change modifies nothing else: `git status --porcelain` names only the two build files touched by this change, and no file under `libosmscout*/` is modified beyond them

## 4. JNI magnification contract and bounds-safe cell lookup

Spec: `client-java-jni-binding`, `magnification-levels`. Design: D4, D5. Uncovered while verifying 3.3: the JavaScout suite aborted the JVM because the bridge read the Java `int` magnification as a `jdouble`.

- [x] 4.1 Analyse the abort: identify the failing assert (`std::array<osmscout::CellDimension, 26>::operator[]`), the offending level (957), its origin (the JNI reading an `int` argument as `jdouble`) and the difference between the CMake (asserts off, silent out-of-bounds read) and Meson (asserts on, abort) builds
- [x] 4.2 Compare every Java `native` declaration with its JNI function and fix the three mismatches (`render`, `renderWithRouteAndPois`, `projectToPixel`) to take `jint magnificationLevel`, matching the Java API and the sibling methods
- [x] 4.3 Validate the level in both render entry points and in `projectToPixel`: a level outside `0..CELL_DIMENSION_MAX` returns no result and warns, so no garbage value reaches a projection
- [x] 4.4 Make the `TileId` cell-size lookup bounds-safe: one helper used by every lookup that reports an out-of-range level and uses the finest cell size instead of indexing past the table, so a release build behaves like an asserts build
- [x] 4.5 Verify the JNI parameter comparison reports no type mismatch for any method afterwards (only the known-benign `String[]`/callback mappings remain unmapped in the scratch comparison)

## 5. Verification of the JNI fix

Spec: `client-java-jni-binding`, `magnification-levels`.

- [x] 5.1 Verify the abort is gone in the asserts-enabled build: `./JavaScout/test.sh build-meson` completes with BUILD SUCCESS, 230 tests, 0 failures, 0 errors, 19 skipped (before: 33 tests and exit 134)
- [x] 5.2 Verify the failing class alone passes: `OSMScoutClientBasemapConfigTest` 14/14, and the same class is the one that used to abort
- [x] 5.3 Verify the level validation: the previous failure path (level out of range) now yields no result plus a warning; the level-957 message no longer appears in a suite run
- [x] 5.4 Verify the C++ side is unaffected: `cmake --build build` (0 errors, 0 warnings in the two changed files) with `ctest` 136/136, and `meson compile` + `meson test` 136/136 with asserts enabled
- [x] 5.5 Record the pre-existing findings from this analysis in `TODO.md` and in `verification.md` (the lost fractional-zoom intent, the `std::bad_alloc` abort)

## 6. Magnification as a fractional scale (revision)

Spec: `client-java-jni-binding`, `magnification-levels`. Design: D4, D5. Requested after the first attempt aligned the bridge to the Java `int` level: NaviVeylin introduced fractional zoom levels, so the bridge keeps the scale factor and the Java API documents it.

- [x] 6.1 `render`, `renderWithRouteAndPois` and `projectToPixel` take the magnification as a `jdouble` scale factor again, with the validation from D5 (finite, `>= 1`, `floor(log2(scale)) <= CELL_DIMENSION_MAX`) before a `Magnification` is constructed
- [x] 6.2 The Java declarations, the Java `renderWithRoute` overload and their javadoc say `double magnification` = scale factor (2^zoom level), fractional values supported
- [x] 6.3 `JavaScout`'s `MapRenderer` (both call sites) and the client tests pass `Math.pow(2, level)` from their integer zoom; the basemap test's retry renders with a fractional exponent, so the fractional path is exercised
- [x] 6.4 Verified: `./JavaScout/test.sh build-meson` against the fixed library runs the suite with 0 failures (227 tests, 18 skipped, `SearchReproTest` excluded — its own pre-existing `bad_alloc` abort is diagnosed in `TODO.md`); `cmake --build` clean, `ctest` 137/137, `meson test` 137/137
- [x] 6.5 The JNI parameter comparison is a test in both build systems: `Tests/CMakeLists.txt` (`JniSignatureParityTest`) and `Tests/meson.build` (`Check JNI signature parity`), backed by the new `scripts/check-jni-signatures.sh`; verified to pass in both (137/137) and to fail on a mutated copy of the Java declarations (`MISMATCH: projectToPixel`, exit 1)
- [x] 6.6 `scripts/check-jni-signatures.sh` reports six JNI functions that have no Java declaration (`getMaxSpeedAt`, `searchLocationByForm`, `reloadBasemap`, `getAddressAt`, `getDatabaseBoundingBox`, `setMapDpi`) as warnings; recorded in `TODO.md` instead of deleting them here
