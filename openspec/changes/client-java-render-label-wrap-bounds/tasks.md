# Tasks

## 1. The bounds as one decision (spec: map-rendering)

- [x] 1.1 Add `libosmscout-client-java/src/render_label_bounds.h`: a dependency-free header (no JNI, no
  libosmscout include) with the four line-wrapping values as one function of the buffer size, in the
  namespace and guard style the sibling helpers of this directory use (`route_step_time.h`,
  `routing_progress_throttle.h`). Verify: the header includes nothing outside the standard library.
- [x] 1.2 Document in the header why the fit-to-width is the shorter buffer side (the buffer carries the pan
  overrun), why it is not the library default (which never binds) and that the other three values are stated
  explicitly so the render does not depend on a default it means to state.
- [x] 1.3 Verify with a host case that the decision uses the width of a tall buffer and the height of a wide
  one, and that its fit-to-width is not the library default of an unconfigured `osmscout::MapParameter`.

## 2. The render applies the decision (spec: map-rendering)

- [x] 2.1 In `libosmscout-client-java/src/OSMScoutClient.cpp`, the shared render body (`renderMapIntoPixels`)
  applies the four values to its `MapParameter` next to `SetFontSize`, with a comment stating why the bound
  is the shorter buffer side. Verify: the render keeps its signature and behaviour otherwise.
- [x] 2.2 Verify no other rendering parameter of the render changed and that the change is confined to that
  render body (both render entry points go through it).

## 3. Host test (spec: map-rendering)

- [x] 3.1 Add `Tests/src/RenderLabelBoundsTest.cpp` with the cases of tasks 1.3: the tall-buffer bound, the
  wide-buffer bound, the four stated values, and the difference from the library default fit-to-width.
- [x] 3.2 Register it in `Tests/CMakeLists.txt` and `Tests/meson.build` after the sibling client-java helper
  tests, with the include path of `libosmscout-client-java/src` and the `OSMScout::Map` target (the default
  parameter lives there). Verify: both build systems build and run it.

## 4. Build and regression verification

- [x] 4.1 Build the native client library and the jar in the CMake build (`osmscout_client_java`, `java_jar`)
  and verify the touched file compiles without errors and without new warnings.
- [x] 4.2 Build the same targets in the Meson build directory if one is configured (`build-meson-check`);
  if the Java client is not configured there, say so instead of reconfiguring the tree.
- [x] 4.3 Rebuild and run the new test through both build systems and verify every case passes.
- [ ] 4.4 Run the JavaScout suite or the client-java-related tests the build allows and compare the failure
  count against `master` before claiming a regression. `OSMScoutClientBasemapConfigTest.testClearingBasemapLookupDirectoryUnloads`
  is a known intermittent flake (passes in isolation): document it, do not fix and do not block on it.
  NOT RUN: the suite does not compile on this machine before any test starts. `mvn -o test` fails reading
  the locally installed `libosmscout-client-java-1.0-SNAPSHOT.jar` from `~/.m2` — `class file has wrong
  version 71.0, should be 61.0` (the jar was built by a newer JDK than the toolchain's Java 17), 100
  compiler errors, no test executed. The jar and the JavaScout sources are not part of this change and the
  working tree does not touch them, so this is an environment precondition, identical on `master`. No
  failure count to compare.
- [x] 4.5 State in `verification.md` what could not be verified: the wrapped line count inside a rendered
  frame is not asserted by a test here. `maps/nordrhein-westfalen` carries a v27 database
  (`db.json` `typeConfigVersion` 27), so a render probe is possible; if one is not run, say so and why.
- [x] 4.6 Run `openspec validate "client-java-render-label-wrap-bounds" --strict` and verify the change
  validates.

## 5. Change hygiene

- [x] 5.1 Verify the diff touches only the files in the proposal's Impact section.
- [x] 5.2 Verify no Java declaration, JNI export or native signature changed (`scripts/check-jni-signatures.sh`
  passes if the tree can run it).
