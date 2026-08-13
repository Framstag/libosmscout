# TextMetricsAll — Tasks

## 1. Measurement API on MapPainter base

- [x] 1.1 Add `TextMetrics` struct (label width/height, per-glyph position + box) and virtual `MeasureText()` with a default empty implementation to `libosmscout-map/include/osmscoutmap/MapPainter.h` (spec: text-metrics-api)
- [x] 1.2 Implement `MeasureText()` in Cairo backend wrapping existing `Layout()`/`ToGlyphs()`/`GlyphBoundingBox()` (spec: text-metrics-api)
- [x] 1.3 Implement `MeasureText()` in AGG backend (spec: text-metrics-api)
- [x] 1.4 Implement `MeasureText()` in Qt backend (spec: text-metrics-api)
- [x] 1.5 Implement `MeasureText()` in Skia backend (spec: text-metrics-api)
- [x] 1.6 Implement `MeasureText()` in SVG backend (spec: text-metrics-api)
- [x] 1.7 Add unit tests for `MeasureText()` on at least one backend: non-empty text returns width/height > 0, N characters return N glyph entries, glyph box coordinates are relative to glyph base point, glyph positions are relative to label origin (spec: text-metrics-api)

## 2. FreeType reference measurement

- [x] 2.1 Add reference measurement helper: load font face via FreeType, set pixel size from `fontSize * fontSizeParam * dpi / 25.4`, per glyph compute ink box from `horiBearingX/Y`, `width`, `height` (26.6 fixed point) and advance (spec: text-metrics-tool)
- [x] 2.2 Add unit tests for the reference helper: ink box values match expected values for a known glyph, pixel size conversion matches the backend formula (spec: text-metrics-tool)

## 3. TextMetricsAll tool

- [x] 3.1 Add `Demos/src/TextMetricsAll.cpp` with CLI parsing (text, fontName, fontSize, dpi, output dir) reusing `DrawMapArgParser` patterns (spec: text-metrics-tool)
- [x] 3.2 Implement per-backend rendering: create painter + canvas, call `MeasureText()`, draw text natively at a common baseline, overlay per-glyph boxes, store PNG to output dir (spec: text-metrics-tool)
- [x] 3.3 Implement terminal dump: per backend print label width/height, per-glyph boxes, and per-glyph difference vs FreeType reference (spec: text-metrics-tool)
- [x] 3.4 Guard each backend block with `HAVE_OSMSCOUT_MAP_*` compile definitions so unavailable backends are skipped with a note, not a failure (spec: text-metrics-tool)
- [x] 3.5 Add `TextMetricsAll` target to `Demos/CMakeLists.txt` following the `DrawMapAll` conditional-backend pattern, linking `Freetype::Freetype` (spec: text-metrics-tool)

## 4. Verification

- [x] 4.1 Build the project with CMake and verify it compiles without errors
- [x] 4.2 Run existing test suite and verify all tests still pass
- [x] 4.3 Run `TextMetricsAll` manually with a sample text and font; verify one PNG per available backend exists, all images draw text at the same baseline, and the terminal dump contains reference + backend values with diffs
- [x] 4.4 Run linters and static analysis on new/modified code (`.clang-tidy`, `.uncrustify`)
- [x] 4.5 Update README/Documentation if tool usage is documented; update AGENTS.md if project structure changes
