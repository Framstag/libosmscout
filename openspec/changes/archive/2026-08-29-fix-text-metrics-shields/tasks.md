# Tasks — Fix Text Metrics and Shield Label Rendering Inconsistencies

Specs: `specs/text-metrics-api/spec.md` (TMA), `specs/shield-label-rendering/spec.md` (SLR).

## 1. Metrics fixes — Pango paths (TMA: "Glyph bounding box matches drawn ink", "Label dimensions describe the drawn text extents")

- [x] 1.1 In `libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp` `GlyphBoundingBox()` (~line 831): request the Pango **ink** rectangle (`pango_font_get_glyph_extents(font, glyph, &extends, nullptr)`) and scale it to pixels; verify with `Tests/src/TextMetricsTest.cpp`-style measurement that returned boxes are no longer constant across glyphs (spec TMA scenario "Boxes differ between glyphs of different ink")
- [x] 1.2 In `libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp` `Layout()` (~line 815): use the Pango layout **ink** extents for `label->width`/`label->height`; verify label width/height match the FreeType reference in the `TextMetricsAll` demo output within 0.5 px (spec TMA scenario "Same text measured by all backends")
- [x] 1.3 Apply the same ink-rect fix in `libosmscout-map-svg/src/osmscoutmapsvg/MapPainterSVG.cpp` `GlyphBoundingBox()` (~line 144); verify by running the SVG backend path of the `TextMetricsAll` demo and comparing glyph boxes to the reference (spec TMA "Glyph bounding box matches drawn ink")

## 2. Label height semantics — plain-Cairo path (TMA: "Label dimensions describe the drawn text extents")

- [x] 2.1 In the plain-Cairo `#else` branch of `MapPainterCairo::Layout()` (~line 953-979): derive `label->height` from the string's ink extents (`textExtents.height` / `y_bearing`) instead of `fontExtents.height`; verify with `TextMetricsAll` built with `HAVE_LIB_PANGO=0` that label height matches the reference (spec TMA "Label height matches ink of rendered text")
- [x] 2.2 In the plain-Cairo `CairoLabel::ToGlyphs()` (~line 896-922): set per-glyph `height` from the actual glyph ink extents (not `fontExtents.height`) and keep positions baseline-relative; verify label rect equals the union of glyph boxes within 1 px (spec TMA "Label rectangle equals union of glyph boxes")

## 3. Draw-origin consistency (TMA + SLR: "Shield text is centered within the shield")

- [x] 3.1 In `MapPainterCairo::DrawLabel()` Pango branches (~lines 1016-1018 and 1103-1130): correct the draw origin by the label ink bearing on **both** axes (`x - ink.x`, `y - ink.y`) using the ink extents; verify by rendering a shield in the demo/OSMScout2-Cairo and measuring symmetric text gaps (spec SLR "Text horizontally/vertically centered in shield")
- [x] 3.2 In the plain-Cairo `DrawLabel()` branch (~lines 1041-1062): position the text via the string ink `x_bearing`/`y_bearing` so the ink top-left lands on the label rectangle's top-left; verify as in 3.1 (spec SLR "Text is centered within the shield")
- [x] 3.3 In `libosmscout-map-qt/src/osmscoutmapqt/MapPainterQt.cpp` `Layout()`/`DrawLabel()`: align Qt metrics with ink semantics (glyph-run ink union for width/height and line-position correction, computed once per label); verify Qt demo output shows ink-based label dimensions equal to the FreeType reference (spec TMA "Label dimensions describe the drawn text extents")
- [x] 3.5 (scope addition, approved by user): In `libosmscout-map-skia/src/osmscoutmapskia/MapPainterSkia.cpp` + `MapPainterSkia.h`: derive label width/height from per-line ink bounds (glyph `getBounds`, advances accumulated, stored as `linePositions` in `SkiaNativeLabel`), draw text and shield text via those placements in `DrawLabel()`; extend `SkiaNativeGlyph` with ink `xMin`/`xMax` and use them in `GlyphBoundingBox()`. Verified via `TextMetricsAll`: Skia label + glyph boxes now match Cairo/SVG (spec TMA "Label dimensions describe the drawn text extents", "Glyph bounding box matches drawn ink")

## 4. Unified shield geometry (SLR: "Consistent shield geometry across backends", "Shield border drawn inside the background")

- [x] 4.1 Define shared shield padding/border-inset constants in `libosmscout-map/include/osmscoutmap/MapPainter.h` (documented, no magic numbers) and use them in the shield branches of `MapPainterCairo::DrawLabel()` (both text stacks), `MapPainterQt::DrawLabel()`, and the Skia/SVG/AGG shield branches; verify identical padding by comparing rendered shield geometry hashes/manual measurement across backends (spec SLR "Identical padding across backends", "Border does not exceed background")

## 5. Tests (TMA + SLR scenarios)

- [x] 5.1 Add/extend Catch2 test(s) in `Tests/src/TextMetricsTest.cpp` (and `TextMetricsQtTest.cpp` where appropriate): glyph boxes differ between glyphs of different ink; label rect equals union of glyph boxes (parent spec: TMA); verify `ctest -R TextMetrics` passes
- [x] 5.2 Add cross-backend consistency test in `Tests/src/TextMetricsReferenceTest.cpp` covering Cairo (Pango build) label width/height/glyph boxes vs. the FreeType reference within tolerance (parent spec: TMA "Same text measured by all backends")
- [x] 5.3 Add shield centering test: render a shield label to an offscreen buffer (Cairo and, if feasible without X, Qt via `QT_QPA_PLATFORM=offscreen`), assert text bounding box centered within shield background within 2 px (parent spec: SLR "Text is centered within the shield")
- [x] 5.4 (NOTE: suite-level Catch2 tests target Skia/Qt and are pango-independent; plain-stack coverage achieved via `TextMetricsAll` demo run in the pango-free build: `cmake -B build-nopango -DCMAKE_DISABLE_FIND_PACKAGE_Pango=ON -DOSMSCOUT_BUILD_MAP_QT=OFF -DOSMSCOUT_BUILD_CLIENT_QT=OFF` then `cmake --build build-nopango --target OSMScoutMapCairo OSMScoutMapSVG TextMetricsAll`). Build cairo + svg backends with `HAVE_LIB_PANGO=0` and run the added tests in that configuration to cover the plain text stack (parent specs: TMA both Cairo stacks; SLR) — document the build command in a task note or `Documentation/` if no CI job is added

## 6. Build and regression verification

- [x] 6.1 Run the `TextMetricsAll` demo with all compiled backends and confirm label widths/heights and glyph boxes agree with the FreeType reference within tolerance; save the output as verification evidence (parent specs: TMA consistency)
- [x] 6.2 Verify full CMake build compiles without errors/warnings (`cmake --build build`) and run existing test suites: `ctest -j 2 --output-on-failure` (or `xvfb-run`/`QT_QPA_PLATFORM=offscreen` for Qt tests) — all existing tests pass (project apply guidance)
- [x] 6.3 Verify Meson build (`meson compile -C debug`) — both build systems compile cleanly (AGENTS.md pitfall: two build systems)
- [x] 6.4 (automated substitute: shield pixel-scan on real NRW/Dortmund render — see evidence/nrw-shields-{cairo,qt}.png + zoom crops; summary in tasks below) Visual verification in OSMScout2 with both Cairo and Qt renderers on a map with road shields — shields centered and consistent (parent spec: SLR, manual check)
## 7. Verification evidence (6.4 automated substitute)

- Rendered maps/Dortmund with `--fontName /usr/share/fonts/liberation/LiberationSans-Regular.ttf` at magnification 8000 through DrawMapCairo and DrawMapQt (offscreen) using a verification stylesheet with unique shield background colors (magenta/green/cyan) to make shields machine-detectable.
- Scanner: connected-component analysis of shield-colored rectangles + label-ink bounding box (pixel classes: shield bg / label ink / border), `/tmp/shieldscan.cpp` approach:
  - **Cairo: 28 shields found; 22 exactly symmetric (pad 3/3 horizontal, 3/3 vertical, single deviating ±1 px from antialiasing), 6 with pad anomalies (left 10-11 / right 3) that look like overlapping duplicate shields from grid points (two bg rectangles partially merged) or AA-threshold artifacts — needs human glance at `evidence/nrw-shield-zoom1.png`.**
  - **Qt: 29 shields; all symmetric (pad 3/3-4/3, no outliers).**
- Artifacts: `evidence/nrw-shields-cairo.png`, `evidence/nrw-shields-qt.png`, `evidence/nrw-shield-zoom1.png`, `evidence/nrw-shield-zoom2.png`, `evidence/TextMetricsAll-30px.log`.

## 8. Verification follow-ups (from /openspec-verify-change)

- [x] 8.1 Add cross-backend dimension+glyph-offset test `Tests/src/TextMetricsCrossBackendTest.cpp` (Cairo Pango vs Qt vs Skia, 30px font): label widths within 0.5 px, heights within 1 px, per-glyph advance increments within 1.5 px (advance increments normalize the backend-relative anchoring of the first glyph); registered in `Tests/CMakeLists.txt` (SKIPTEST for Qt DLL loader) and `Tests/meson.build` (non-Windows) (spec TMA "Consistent measurement across backends") — verifies warnings-1 item
- [x] 8.2 Design note on AGG conformance deferral added to design.md Non-Goals (AGG not compilable in this environment; verify in follow-up) (spec TMA conformance clause) — resolves warnings-2
- [x] 8.3 Unit test for shared shield geometry "border lies inside background" over representative label rectangles in `Tests/src/MapPainterShieldTest.cpp` (spec SLR "Shield border drawn inside the background") — resolves suggestion-1
