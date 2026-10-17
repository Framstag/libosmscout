# Verification

Branch `client-java-render-label-wrap-bounds`, base `origin/master`.

## What the change is

The Java/JNI render states its four label line-wrapping values instead of inheriting them: minimum character
count 5, maximum 15, fit-to-area enabled, and fit-to-width the shorter side of the render buffer. The four
values come from one dependency-free header (`libosmscout-client-java/src/render_label_bounds.h`,
`naviveylin::LabelLineBoundsForBuffer`) that the shared render body `renderMapIntoPixels` and the host test
both read, so the applied values cannot drift from the stated ones.

Port of the reference hunk `e303556e2` (13 added lines in `renderMapIntoPixels`), extended by the
dependency-free decision and its host test the change's spec asks for. The reference sets all four values
explicitly although three of them coincide with the library defaults (min 5 / max 15 / fit-to-area true in
`libosmscout-map/src/osmscoutmap/MapParameter.cpp:37-39`); only the fit-to-width changes, from the
non-binding default of 8000 px (line 40) to the shorter buffer side. The explicit setting of all four is
kept so the render does not depend on a default it means to state.

The fit-to-width is `std::min(width, height)` with no division by the canvas overrun, because this buffer is
the one the caller asked for and already carries the pan overrun. The Qt plane renderer
(`libosmscout-client-qt/src/osmscoutclientqt/PlaneMapRenderer.cpp:376-379`) states the same character bounds
and fit-to-area and divides its projection canvas side by `canvasOverrun`, which its canvas does not carry.

## Commands run

1. `cmake --build /home/tim/projects/libosmscout/build --target RenderLabelBoundsTest`
   → CMake regenerated the build files (new test registered), compiled
   `Tests/CMakeFiles/RenderLabelBoundsTest.dir/src/RenderLabelBoundsTest.cpp.o`, linked
   `build/Tests/RenderLabelBoundsTest-1.1.1` and the `RenderLabelBoundsTest` symlink. No warnings.

2. `/home/tim/projects/libosmscout/build/Tests/RenderLabelBoundsTest`
   → `All tests passed (8 assertions in 3 test cases)`

3. `cmake --build /home/tim/projects/libosmscout/build --target osmscout_client_java java_jar`
   → 18 steps; `Compiling Java sources and generating JNI headers`, `Packaging Java classes into jar`,
   `Building CXX object libosmscout-client-java/CMakeFiles/osmscout_client_java.dir/src/OSMScoutClient.cpp.o`,
   `Linking CXX shared library libosmscout-client-java/libosmscout_client_java.so`. No warnings from the
   touched translation unit; the JAR was rebuilt.

4. `ninja -C /home/tim/projects/libosmscout/build-meson-check build.ninja`
   → regenerated the Meson build files of the existing `build-meson-check` directory (Java client configured:
   `libosmscout-client-java : true`, `Java support : true`), 181 targets.

5. `meson compile -C /home/tim/projects/libosmscout/build-meson-check RenderLabelBoundsTest`
   → `Compiling C++ object Tests/RenderLabelBoundsTest.p/src_RenderLabelBoundsTest.cpp.o`, `Linking target
   Tests/RenderLabelBoundsTest`.

6. `/home/tim/projects/libosmscout/build-meson-check/Tests/RenderLabelBoundsTest`
   → `All tests passed (8 assertions in 3 test cases)`

7. `ninja -C /home/tim/projects/libosmscout/build-meson-check libosmscout-client-java/src/libosmscout_client_java.so.1.1.1 libosmscout-client-java/java/libosmscoutclientjava.jar`
   → the same two targets as the Meson build names `osmscoutclientjava` / `osmscoutclientjavaJar`. Built:
   `Compiling C++ object libosmscout-client-java/src/libosmscout_client_java.so.1.1.1.p/OSMScoutClient.cpp.o`
   and `Creating JAR libosmscout-client-java/java/libosmscoutclientjava.jar`. No warning from
   `OSMScoutClient.cpp`; the 70 Java warnings are pre-existing javadoc warnings in
   `BasemapManager.java`, `CurrentRoadInfo.java`, `OSMScoutClientBuilder.java`, `PoiCategories.java`.
   `meson compile -C <dir> <target>` cannot address these two targets by name (`Can't invoke target ...:
   target not found` — Meson only knows the introspected target ids), so the underlying `ninja` target paths
   are named instead.

8. `openspec validate client-java-render-label-wrap-bounds --strict`
   → `Change 'client-java-render-label-wrap-bounds' is valid`

9. `bash scripts/check-jni-signatures.sh`
   → `JNI signatures match: 59 native declarations checked, 5 dead JNI functions reported above.`
   (The five dead functions are pre-existing; no Java declaration, JNI export or native signature is part of
   this change — the diff adds no `JNIEXPORT` function and no Java source line.)

10. `mvn -o -f JavaScout/pom.xml test -Dnative.lib.dir=/home/tim/projects/libosmscout/build/libosmscout-client-java`
    → BUILD FAILURE during `maven-compiler-plugin:compile`, 100 errors, no test executed: every JavaScout
    source that imports the client reads the locally installed
    `~/.m2/repository/net/sf/libosmscout/libosmscout-client-java/1.0-SNAPSHOT/libosmscout-client-java-1.0-SNAPSHOT.jar`,
    whose classes are `class file has wrong version 71.0, should be 61.0` — the jar was built by a newer JDK
    than the Java 17 toolchain Maven runs on. Both inputs of that failure (the installed JAR and the JavaScout
    sources) are untouched by this change, the failure happens before any JVM loads the native library, and it
    is therefore identical on `master`. The JavaScout suite could not be run and no failure count was
    compared.

## Not run, and why

- **No rendered-frame probe.** The line count a label is wrapped into inside a rendered frame is not asserted
  here; the spec's scenarios that need a frame ("A long multi-word label is wrapped", "A one-word label stays
  on one line", "A short label is not wrapped", "The bound follows the buffer the caller asked for") are not
  covered by an executed test. A probe is possible: `maps/nordrhein-westfalen/db.json` states
  `typeConfigVersion` 27, so a v27 database is present on this machine. It was not run because no probe
  harness is part of this change's Impact section — one would need a new render binary or test target outside
  the changed files — and the change is confined to the four values a `MapParameter` receives, which the host
  test pins. The database was not imported (not needed).
- **No JavaScout suite run** (see command 10 above): the suite does not compile on this machine before any
  test starts. `OSMScoutClientBasemapConfigTest.testClearingBasemapLookupDirectoryUnloads`, the known
  intermittent flake, was therefore not observed either; it is neither fixed nor blocking here.
- **No Meson reconfiguration of the tree.** The existing `build-meson-check` directory was used, with its
  build files regenerated automatically by `ninja`; no new Meson setup was created.
- **No `scripts/format-check.sh`**: it is red repo-wide on this machine, including pristine `master` files.
  The surrounding style was matched by hand.

## Manual checks

- `git status --porcelain` after the work: `Tests/CMakeLists.txt`, `Tests/meson.build`,
  `libosmscout-client-java/src/OSMScoutClient.cpp` modified; `Tests/src/RenderLabelBoundsTest.cpp`,
  `libosmscout-client-java/src/render_label_bounds.h` added; the change directory
  `openspec/changes/client-java-render-label-wrap-bounds/` added. `build-nopango/`, `maps/repository/` and
  other unit directories stay untracked and unstaged.
- `renderMapIntoPixels` keeps its signature; the added lines are the four `SetLabelLine*` calls and the
  decision read, and no other rendering parameter of that body changed. Both render entry points
  (`renderWithRouteAndPois`, `renderInto`) run this one body.
