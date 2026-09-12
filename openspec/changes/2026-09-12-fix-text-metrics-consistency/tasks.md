## 1. SVG backend: real metrics in the non-Pango path

Spec: `text-metrics-consistency` — "SVG backend text metrics are independent of Pango availability"

- [ ] 1.1 Add FreeType-based measurement to the SVG backend's non-Pango `Layout()`, `ToGlyphs()`, `GlyphBoundingBox()` (`libosmscout-map-svg/src/osmscoutmapsvg/MapPainterSVG.cpp`): per-glyph advance from glyph metrics, ink box from glyph extents, height from ink bbox; replace `AverageCharacterWidth`-based width/advance computation
- [ ] 1.2 Wire FreeType into `libosmscout-map-svg` in both build systems (CMake `freetype` target / meson `ftDep`), mirroring `libosmscout-map-cairo`; verify both configurations configure and compile cleanly
- [ ] 1.3 Verify the Pango path is untouched: Pango-enabled build measures the same label/text/font/size with byte-identical metrics to the pre-change build (compare against baseline recorded in the `fix-text-metrics-shields` evidence)
- [ ] 1.4 Verify no-Pango build measurement: build SVG backend without Pango, measure the 30 px label from the spec scenario, assert width within 10% of ink width (216 px reference, currently 340 px)

## 2. Unit test: SVG non-Pango measurement

Spec: `text-metrics-consistency` — "Measurement drift is caught by the test suite"

- [ ] 2.1 Add a text metrics test for the SVG backend (extend `TextMetricsCairoTest` pattern or add a sibling test) that compares non-Pango SVG measurements against the `TextMetricsAll` reference within the defined margins
- [ ] 2.2 Add a parity comparison between a Pango and a non-Pango measurement of the same text where both can be built (compile-time guarded), asserting the ≤10% consistency margin; verify the test fails on the pre-change approximation (340 px) to prove it detects drift

## 3. TextMetricsAll reference: ink height

Spec: `text-metrics-consistency` — "TextMetricsAll reference reports ink height"

- [ ] 3.1 In `Demos/src/TextMetricsAll.cpp`, compute the reference label height from the ink bounding box of the rendered glyphs instead of `face->size->metrics.height`; verify the reference reports 23 px (not 35 px) for the 30 px font scenario
- [ ] 3.2 Verify `TextMetricsCairoTest` (and any test using the `TextMetricsAll` reference) passes with unchanged margins against the new ink-based reference

## 4. Integration verification

Spec: `text-metrics-consistency` — all requirements

- [ ] 4.1 Rebuild both build systems in Pango-enabled and no-Pango configurations; verify no compile errors
- [ ] 4.2 Run the full test suite (`ctest -j 2 --output-on-failure`, `meson test --timeout-multiplier 2 -C build --print-errorlogs`) and verify all text metrics, shield, and label tests pass
- [ ] 4.3 Update any docs referencing the `TextMetricsAll` reference semantics or the SVG non-Pango approximation (TODO.md, `text-metrics-api` evidence notes) to reflect ink semantics and real metrics
