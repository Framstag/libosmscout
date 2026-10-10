# Verification

## Commands run

- `meson compile -C build-meson TextMetricsQtTest` — clean, no warning for
  `libosmscout-map-qt/src/osmscoutmapqt/MapPainterQt.cpp` or `Tests/src/TextMetricsQtTest.cpp`.
- `meson test --no-rebuild -C build-meson "Check TextMetricsQt compilation"` — OK,
  `All tests passed (90 assertions in 6 test cases)`.
- `cmake --build build -j 8` — full build, 323 targets, no warning for the touched files.
- `cmake --build build --target TextMetricsQtTest` — the CMake build compiles the test
  (its registration keeps the project's `SKIPTEST` convention, see Limitations).
- `cd build && QT_QPA_PLATFORM=offscreen ctest -j 4 --output-on-failure --exclude-regex 'PerformanceTest'`
  — `100% tests passed out of 105`, 39 s.
- `meson test --no-rebuild -C build-meson "Check TextMetricsQt compilation" "Check MapPainterShieldQt compilation" "Check TextMetrics compilation"`
  — 3/3 OK.

## Scenario traceability

All scenarios are in `openspec/changes/qt-font-cache-identity-and-release/specs/painter-font-cache/spec.md`;
all cases are in `Tests/src/TextMetricsQtTest.cpp`.

| Requirement | Scenario | Case or observation |
|---|---|---|
| identity of a cached font … at the resolution of the drawing | Sizes that differ by less than a device pixel share one font | case `Qt painter shares one resolved font for sizes within a device pixel` |
| identity … | Many distinct automatic label sizes resolve no font per label | case `Qt painter resolves no font per label and none for a repeated frame` |
| identity … | Repeated frames under unchanged parameters resolve no further font | same case, second render loop |
| resolved fonts a painter retains are released | An explicit release drops the retained fonts | case `Qt painter releases the resolved fonts it retains` |
| resolved fonts … are released | A stylesheet reload releases the fonts of the replaced stylesheet | case `Qt painter releases the fonts of a replaced stylesheet` |
| releasing a cached font does not change the rendered output | A font resolved again after a release draws as it did before | case `Qt painter draws a label as before after releasing its fonts` |
| releasing … does not change the rendered output | The measurement and the drawn font of a frame agree under the identity rule | the metric and glyph equality block of the identity case (a second size in the same pixel step measures to the same dimensions and glyph boxes) |
| retained resolved fonts … are observable | A test reads the number of resolved fonts | every case reads `GetResolvedFontCount()`; the case `Qt painter resolves no font per label and none for a repeated frame` asserts it is unchanged by a repeated frame |
| retained resolved fonts … are observable | A test reads the number of retained fonts | case `Qt painter releases the resolved fonts it retains`: retained count after the release is zero, and the next draw resolves the font again |

## Falsification of the cases (mutation checks)

Each case was shown to fail when the behaviour it guards is removed from the implementation, then the
implementation was restored and the case re-run green:

| Mutation | Result |
|---|---|
| `FontPixelSize` returns a fine-grained value (`scaledFontSize*1000.0`) instead of the pixel grid | identity cases fail at `TextMetricsQtTest.cpp:196` and `:222`; 2 of 6 cases fail |
| `MapPainterQt::ReleaseFonts` drops nothing | release case fails at `TextMetricsQtTest.cpp:256`; 1 of 6 cases fail |
| `MapPainterQt::StyleSheetChanged` does not release the fonts | reload case fails at `TextMetricsQtTest.cpp:328`; 1 of 6 cases fail |

## Rendering is unchanged

- The identity rule keeps the resolution the Qt painter already had: `FontDescriptor::fontSize` was
  and is the truncated product, so the fonts a frame resolves and draws with are the same as before
  this change (the operation is now named instead of an implicit narrowing conversion).
- `GetFont` returns the same `QFont` for the same identity; the cases assert the measured dimensions
  and the per-glyph boxes of a label are equal across a release.
- No render comparison against a stored image was made: the Qt text-metrics target does not draw a
  world view, and the change touches no drawing code. A pixel-level comparison would have to render
  `maps/Dortmund` through the Qt painter, which no test target of this repository does today.

## Limitations

- `TextMetricsQtTest` is registered as `SKIPTEST` in `Tests/CMakeLists.txt` (project convention for Qt
  tests, because the Windows CMake CI cannot load the Qt DLLs) and runs under Meson on non-Windows
  hosts. The cases added here therefore execute in the Meson run above; the CMake build only compiles
  them. This is the pre-existing divergence of TODO §76 and is not changed here.
- `PerformanceTest` is excluded from the full `ctest` run, as in the repository's sanitizer job; it is
  not affected by this change (no change to the drawing or data paths).
- The full Meson test suite was not run: `meson test` over the whole project takes far longer than the
  change's touchpoint justifies. The Qt and text-metrics targets of the touchpoint were run instead.
- `cspell` is not installed on this machine, so the spell check of the touched files could not be run.
  The added words are ordinary English plus identifiers already present in the files.
- The exact drawn pixels are not compared (see above); the drawn glyph boxes and the measured
  dimensions are.
