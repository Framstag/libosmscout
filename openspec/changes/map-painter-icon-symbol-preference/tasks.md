# Tasks

## 1. Render parameter (spec: icon-symbol-preference)

- [x] 1.1 Add the preference to `libosmscout-map/include/osmscoutmap/MapParameter.h`: the member, a
  `Set…` setter and a `Get…` getter, each documented with Doxygen, stating that it applies to an entry that
  carries both renderings and that the default is the raster icon. Verify: the header documents the
  default and the two-rendering condition.
- [x] 1.2 Set the documented default in `libosmscout-map/src/osmscoutmap/MapParameter.cpp` and implement
  the setter. Verify: a default-constructed `MapParameter` reports the raster icon preference.

## 2. Frame preparation (spec: icon-symbol-preference)

- [x] 2.1 In `libosmscout-map/src/osmscoutmap/MapPainter.cpp`, hoist the symbol lookup out of the fallback
  branch and make the point label stage push a `Symbol` label data when the preference is set and the
  entry carries a symbol, else the existing raster icon branch, else the existing symbol fallback. Verify:
  a two-rendering entry registers exactly one label data, and an entry with a single rendering behaves as
  before.
- [x] 2.2 Verify the reordering kept the fallback: an entry whose raster icon cannot be served but which
  carries a symbol still registers the symbol under the default. Verify: the corresponding test case
  passes.

## 3. Host test (spec: icon-symbol-preference)

- [x] 3.1 Add `Tests/src/MapPainterIconSymbolPreferenceTest.cpp`: drive the frame preparation up to and
  including the node label step with a no-op painter that decides whether the icon image counts as
  available and records the label data registered. Verify: the test needs no backend, no image file and no
  database.
- [x] 3.2 Cover the default (raster icon registered, no symbol), the preference on a servable raster icon
  (symbol registered, no icon), the preference on an unservable raster icon (symbol registered), the
  unservable raster icon under the default (symbol fallback), a symbol-only entry and an icon-only entry
  whose image cannot be served (no label). Verify: every scenario of the capability has a case.
- [x] 3.3 Cover that the preference is a property of the render: a frame prepared with the default keeps
  its raster icon label after the parameter of a later render is changed. Verify: the case fails if the
  preference is stored outside the render parameters.
- [x] 3.4 Register the test in `Tests/CMakeLists.txt` and `Tests/meson.build`, gated on the map library
  being built. Verify: both build systems build and run it.

## 4. Java client (spec: icon-symbol-preference)

- [x] 4.1 Add the native entry point in `libosmscout-client-java/src/OSMScoutClient.cpp`: store the value
  with the client handle and read it when the next frame's render parameters are built, so no stylesheet
  reload is needed. Verify: no render path reads a stale value after the setter.
- [x] 4.2 Add the declaration and Javadoc to
  `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java`, stating the default,
  the two-rendering condition and that the value takes effect on the next frame. Verify: the declaration
  exists, so the native symbol is reachable from Java.

## 5. Build and regression verification (spec: icon-symbol-preference)

- [x] 5.1 Build the CMake build with the map library, the Java client library and the tests, and verify it
  compiles without errors and without warnings from the touched files.
- [x] 5.2 Build the Meson build the same way and verify it compiles without errors.
- [x] 5.3 Run the new test through both build systems and verify every case passes.
- [x] 5.4 Run the rest of the suite and verify no existing test regresses.
- [x] 5.5 Run `openspec validate "map-painter-icon-symbol-preference" --strict` and verify the change
  validates.

## 6. Documentation and change hygiene

- [x] 6.1 Verify the new public API is documented: the parameter's contract, its default and the
  two-rendering condition, in the header and in the Java declaration.
- [x] 6.2 Verify no backend (`libosmscout-map-*`) changed and no stylesheet or type-definition file is in
  the diff.
- [x] 6.3 Verify the diff touches only the files listed in the proposal's Impact section.

## 7. Verification evidence

Exact commands, results and the one deviation (the test cases the tasks require beyond the reference
branch) are recorded in `verification.md`.
