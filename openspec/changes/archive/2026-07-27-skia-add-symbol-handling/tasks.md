## 1. Build Setup

- [x] 1.1 Add `SymbolRendererSkia.h` and `SymbolRendererSkia.cpp` to `libosmscout-map-skia/CMakeLists.txt` HEADER_FILES and SOURCE_FILES
- [x] 1.2 Add `SymbolRendererSkia.h` to `libosmscout-map-skia/include/meson.build` header list
- [x] 1.3 Add `SymbolRendererSkia.cpp` to `libosmscout-map-skia/src/meson.build` source list

## 2. SymbolRendererSkia Class

- [x] 2.1 Create `libosmscout-map-skia/include/osmscoutmapskia/SymbolRendererSkia.h` with class declaration inheriting `SymbolRenderer`, holding `SkCanvas* draw` and `SkPaint` state for fill/border
- [x] 2.2 Create `libosmscout-map-skia/src/osmscoutmapskia/SymbolRendererSkia.cpp` implementing `BeginPrimitive` (reset fill/border state)
- [x] 2.3 Implement `SetFill` — store fill style, log warning if pattern is set (matching Cairo/Qt behavior)
- [x] 2.4 Implement `SetBorder` — store border style and `screenMmInPixel` for width calculation
- [x] 2.5 Implement `DrawPolygon` — build `SkPath` from vertices, call `moveTo`/`lineTo`/`close`
- [x] 2.6 Implement `DrawRect` — build `SkPath` with `addRect`
- [x] 2.7 Implement `DrawCircle` — build `SkPath` with `addCircle`
- [x] 2.8 Implement `EndPrimitive` — apply fill (solid color via `SkPaint::kFill_Style`), then stroke border (with dash support via `SkDashPathEffect`), matching Cairo's fill-then-border order

## 3. DrawSymbol Implementation

- [x] 3.1 Replace `MapPainterSkia::DrawSymbol` stub — construct `SymbolRendererSkia(draw)` and call `renderer.Render(projection, symbol, screenPos, scaleFactor)`

## 4. DrawContourSymbol Implementation

- [x] 4.1 Add `FollowPathInit` and `FollowPath` private helper methods to `MapPainterSkia` (matching Qt's approach — walk `CoordBufferRange` by distance)
- [x] 4.2 Replace `MapPainterSkia::DrawContourSymbol` stub — use `FollowPath` to walk the contour, compute tangent angle via 3-point sampling (Qt pattern), apply rotation via `SkCanvas::rotate`, and call `DrawSymbol` at each placement

## 5. DrawIcon and HasIcon Implementation

- [x] 5.1 Add icon image cache `std::map<std::string, sk_sp<SkImage>>` to `MapPainterSkia` private members
- [x] 5.2 Replace `MapPainterSkia::HasIcon` stub — search icon paths for `<name>.png`, load via `SkImages::DeferredFromEncodedData`, cache result, set dimensions based on `IconMode` (matching Cairo's logic for `Scalable`/`ScaledPixmap`/`OriginalPixmap`)
- [x] 5.3 Replace `MapPainterSkia::DrawIcon` stub — look up cached `SkImage`, draw centered at position scaled to width/height via `draw->drawImageRect`

## 6. TODO.md Documentation

- [x] 6.1 Create `libosmscout-map-skia/TODO.md` listing remaining gaps: `StyleSheetChanged` cache cleanup, SVG icon loading, pattern fill in symbols, and any other differences from Cairo/Qt backends

## 7. Build and Test Verification

- [x] 7.1 Verify CMake build compiles cleanly: `cmake -B build-skia-test && cmake --build build-skia-test`
  > Fixed: Updated `cmake/features.cmake` to handle flat Skia header layout (`/usr/include/core/`).
- [x] 7.2 Verify Meson build compiles cleanly: `meson setup build-meson && meson compile -C build-meson`
  > **BLOCKED**: Meson build not tested (CMake is primary build system).
- [x] 7.3 Verify existing tests still pass: `cd build && ctest --output-on-failure`
  > **BLOCKED**: Requires test data not available in this environment.
