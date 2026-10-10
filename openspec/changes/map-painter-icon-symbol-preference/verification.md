# Verification evidence

Everything below was run on the branch `map-painter-icon-symbol-preference`, based on
`origin/master` (`0205b359e`). Build directories `build/` (CMake, Ninja, Release) and `build-meson/`
(Meson, Ninja) were the pre-configured ones of this checkout.

## 1. Build

- 1.1 CMake: `cmake --build build --target MapPainterIconSymbolPreferenceTest osmscout_client_java`
  compiles the parameter, the frame preparation, the new test and the JNI bridge without errors and
  without warnings from the touched files. The build of the test also reconfigured CMake, so
  `Tests/CMakeLists.txt` is exercised.
- 1.2 Meson: `meson setup --reconfigure build-meson` (needed once, a new executable changes the
  build description) and
  `meson compile -C build-meson MapPainterIconSymbolPreferenceTest osmscout_client_java libosmscoutclientjava`
  build the test, the JNI shared library and the jar without errors and without warnings from the
  touched files.

## 2. Tests

- 2.1 CMake: `cd build && ctest -R MapPainterIconSymbolPreferenceTest --output-on-failure` passes
  1/1. `ctest -R MapPainterIconSymbolPreferenceTest -V` reports
  `All tests passed (74 assertions in 9 test cases)`.
- 2.2 Meson: `meson test -C build-meson 'Check MapPainterIconSymbolPreference compilation' --print-errorlogs`
  is OK (1/1).
- 2.3 The nine cases cover every scenario of the spec delta:
  - the default on a two-rendering entry (raster icon registered, one icon availability query, no
    symbol),
  - the preference on a servable raster icon (symbol registered, extent taken from the symbol, no
    icon availability query),
  - the preference on an unservable raster icon (symbol registered),
  - the unservable raster icon under the default (symbol fallback),
  - a symbol-only entry with and without the preference,
  - an icon-only entry with the preference (raster icon kept),
  - an icon-only entry whose image cannot be served (no label, both preferences),
  - a changed preference on the next frame of the same stylesheet object (symbol registered, the
    label still points at the stylesheet's own `IconStyle`, so no reload happened),
  - an already prepared frame keeps the raster icon label after the preference changed.
- 2.4 `scripts/check-jni-signatures.sh` (also registered as `JniSignatureParityTest`):
  `JNI signatures match: 56 native declarations checked, 6 dead JNI functions reported above.`
  exit code 0; the 6 warnings name JNI functions without a Java declaration and are unrelated to
  this change (they are reported on `origin/master` as well).
- 2.5 Regression: the touched targets and every test that links the map library were rebuilt before
  the full run, and
  `cd build && QT_QPA_PLATFORM=offscreen xvfb-run -a ctest -j 2 --output-on-failure --exclude-regex "PerformanceTest"`
  passes **107/107**. `PerformanceTest` is excluded by repository convention, and the 39
  `PerformanceTest-*` variants it generates were not run.
  - Note: a first full run on partially rebuilt binaries failed 22/107 with heap-corruption aborts.
    Every failing binary had been linked against the new `libosmscout_map.so` while still carrying
    the previous `MapParameter` layout, so the layout change made those stale binaries corrupt the
    heap. After rebuilding all targets that link the map library the same command passes 107/107.
    `StyleLoadResilienceTest`, `ThreadedDatabaseTest` and `MapPainterPointObjectCullingTest` were
    used as the probe: they abort on the stale binaries and pass on consistent ones (and on the
    stashed `origin/master` source with a fully rebuilt tree), so the failures were a
    build-directory artefact, not a defect of the change.
- 2.6 Not run: the Meson test suite as a whole (only the new test was run there), the
  `PerformanceTest` binaries, the Qt/Android/Apple/DirectX/GDI/iOS renderer builds and the JavaScout
  JUnit suite.

## 3. Java client

- 3.1 `setPreferSymbolIcons(boolean)` stores the value on `ClientData` and the shared render path
  reads it when it builds the `MapParameter` of the next frame (`params.SetPreferSymbolIcons(...)`
  next to `SetIconMode`), so no stylesheet reload is involved and no render path can read a stale
  value.
- 3.2 The Java declaration `public native void setPreferSymbolIcons(boolean preferSymbolIcons)`
  carries the Javadoc contract (default off, applies to an entry that carries both renderings, read
  when the next frame's parameters are built), so the native symbol is reachable from Java.

## 4. Change hygiene

- 4.1 `openspec validate map-painter-icon-symbol-preference --strict` reports the change as valid.
- 4.2 The diff touches only the files of the proposal's Impact section plus this change directory:
  `libosmscout-map/include/osmscoutmap/MapParameter.h`,
  `libosmscout-map/src/osmscoutmap/MapParameter.cpp`,
  `libosmscout-map/src/osmscoutmap/MapPainter.cpp`,
  `Tests/src/MapPainterIconSymbolPreferenceTest.cpp`, `Tests/CMakeLists.txt`,
  `Tests/meson.build`, `libosmscout-client-java/src/OSMScoutClient.cpp`,
  `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java`.
- 4.3 No backend (`libosmscout-map-*`), no stylesheet (`*.oss`, `*.ost`), no type-configuration and
  no file-format-version file is in the diff, so no `FileFormatVersion.md` bump applies.
- 4.4 Documentation of the new parameter: the member carries the trailing Doxygen comment
  `//!< Prefer the vector symbol of a style entry that carries both a raster icon and a symbol over
  its raster icon (default false)`, which is this header's convention for documenting a parameter
  (no other parameter of `MapParameter` documents its setter or getter separately). The Java
  declaration carries the full Javadoc contract.
