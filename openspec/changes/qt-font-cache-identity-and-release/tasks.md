# Tasks

## 1. The Qt painter states the identity rule and exposes what it resolved

- [x] 1.1 State the identity rule of the resolved-font cache in `libosmscout-map-qt/include/osmscoutmapqt/MapPainterQt.h` where `FontDescriptor` and `QMap<FontDescriptor,QFont> fonts` are declared: the requested font name, the font size quantized to the pixel grid of the drawing, the weight and the italic flag are the inputs a cached font is selected by (spec: `painter-font-cache`, *identity … at the resolution of the drawing*).
  Verify: the declarations carry the rule and `cmake --build build --target OSMScoutMapQt` compiles.
- [x] 1.2 Make the rounding to the pixel grid an explicit, named operation of `MapPainterQt::GetFont` (`libosmscout-map-qt/src/osmscoutmapqt/MapPainterQt.cpp:61-84`) instead of the implicit narrowing into `FontDescriptor::fontSize`, keeping today's resolution and behaviour, and delete the stale `// TODO: Clean up fonts` (`:58`) or replace it with a reference to the rule (spec: *identity … at the resolution of the drawing*).
  Verify: `QT_QPA_PLATFORM=offscreen ctest -R TextMetricsQt --output-on-failure` passes and a render of `maps/Dortmund` at magnification level 15 with the shipped stylesheet is pixel-identical to the same render from the branch point.
- [x] 1.3 Expose the observable diagnostics of the cache on `MapPainterQt` — the number of fonts resolved rather than served and the number of fonts currently retained — in the shape of `MapPainterCairo::GetResolvedFontCount()` (`libosmscout-map-cairo/include/osmscoutmapcairo/MapPainterCairo.h:335`), documented where declared (spec: *retained resolved fonts are observable*).
  Verify: the target compiles and a new case in `Tests/src/TextMetricsQtTest.cpp` reads both values after a render; `openspec validate qt-font-cache-identity-and-release` still reports the change valid.
- [x] 1.4 Add the cases for the identity rule: two resolved font sizes that differ by less than a device pixel share one font, a view with many distinct automatic sizes resolves no font per label, and a repeated frame resolves no further font (spec: *identity … at the resolution of the drawing*, all three scenarios).
  Verify: the new cases pass with `QT_QPA_PLATFORM=offscreen ctest -R TextMetricsQt --output-on-failure`, and the assertions use the counters of task 1.3 rather than timings.
- [x] 1.5 Document the identity rule and the counters on the declarations (doxygen), so task 4.1 has the header as its single place (spec: *identity … at the resolution of the drawing*, *retained resolved fonts are observable*).
  Verify: the declarations of `FontDescriptor`, `fonts` and the two readers state the rule and the meaning of each value; the project's spell check passes on the touched files.

## 2. The Qt painter releases the fonts it retains

- [x] 2.1 Add the release to the Qt painter: a public method that drops every resolved font it retains, called by `MapPainterQt::StyleSheetChanged` (`MapPainterQt.cpp:886-891`, which already drops the pattern images of the replaced stylesheet) and by the destructor (`:55-59`), documented where declared, mirroring the stylesheet release the Cairo painter has (`libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp:937-971`) (spec: *resolved fonts a painter retains are released*, both scenarios).
  Verify: `cmake --build build --target OSMScoutMapQt` compiles, the method's declaration states that it drops every retained font, and `Tests/src/MapPainterShieldQtTest.cpp` still passes with `QT_QPA_PLATFORM=offscreen`.
- [x] 2.2 Add the cases for the release: an explicit release leaves no retained font, the font a later draw needs is resolved again rather than served, and the drawn output is as before; and after a stylesheet reload the fonts of the replaced stylesheet are resolved again (spec: *resolved fonts a painter retains are released*, *explicit release* and *stylesheet reload* scenarios).
  Verify: the cases pass with `QT_QPA_PLATFORM=offscreen ctest -R TextMetricsQt --output-on-failure`, asserting the retained count, the resolved count and the redrawn pixmap.
- [x] 2.3 Add the case that a release does not change what is drawn: a label drawn before the release and the same label drawn afterwards, after the font was resolved again, produce the same glyph boxes and the same measured dimensions (spec: *releasing a cached font does not change the rendered output*, both scenarios).
  Verify: the case passes with `QT_QPA_PLATFORM=offscreen ctest -R TextMetricsQt --output-on-failure`, comparing the drawn glyph boxes and the measured dimensions of the two drawings.

## 3. Build-system test parity

- [x] 3.1 Keep both build systems' test rosters in step for anything this change registers: the new cases live in the existing `TextMetricsQtTest` target, and only a new target has to be added to both `Tests/CMakeLists.txt` and `Tests/meson.build` (the divergence recorded as TODO §76).
  Verify: the Qt text-metrics test names reported by `ctest -N` in the CMake build and by `meson test --list -C build-meson` agree.

## 4. Documentation of the cache contract

- [x] 4.1 Document the contract where the cache is declared and in the accessors: the identity rule, the two release points and the meaning of the two counters (spec: all four requirements of `painter-font-cache`).
  Verify: reading `libosmscout-map-qt/include/osmscoutmapqt/MapPainterQt.h` and the release sites in `MapPainterQt.cpp` shows the rule, both release points and both counters, and no comment still claims a cache bound.
- [x] 4.2 Record the outcome for the TODO entry: the Qt half of TODO §33 (identity rule and release) is addressed by this change, and the Cairo, SVG, IOS and DirectX caches remain as follow-ups — their unquantized or size-only identity, their differing release points, and the sub-pixel rendering change a quantization brings to a backend that draws fractional font sizes. The stale `// TODO: Clean up fonts` in `MapPainterAgg.cpp:49` also belongs there.
  Verify: `TODO.md` no longer claims the Qt painter's cache is neither released nor identity-ruled, and the remaining backends and the AGG TODO are named in entries of their own.

## 5. Integration verification

- [x] 5.1 Both build systems compile the touched files without warnings.
  Verify: `cmake --build build` and `meson compile -C build-meson` complete and report no warning for `MapPainterQt.cpp`/`MapPainterQt.h`.
- [x] 5.2 The existing test suite passes, so nothing regressed outside the touchpoint.
  Verify: `cd build && QT_QPA_PLATFORM=offscreen ctest -j 2 --output-on-failure` (or `xvfb-run`) and `meson test --timeout-multiplier 2 -C build-meson --print-errorlogs` both report the same failures as before the change (none expected).
- [x] 5.3 Write the change's `verification.md` and check the traceability the archive requires: every scenario of `specs/painter-font-cache/spec.md` maps to a case or an observation, and every case maps to a requirement.
  Verify: `openspec validate qt-font-cache-identity-and-release --strict` passes and `verification.md` names the case or command behind each of the nine scenarios.
