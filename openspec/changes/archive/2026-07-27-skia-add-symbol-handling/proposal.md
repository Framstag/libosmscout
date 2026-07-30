## What Changes

The Skia map backend (`libosmscout-map-skia`) currently has three stub/no-op methods: `DrawSymbol`, `DrawContourSymbol`, and `DrawIcon`. This change implements all three by:

1. Creating a `SymbolRendererSkia` class (analogous to `SymbolRendererCairo` and `SymbolRendererQt`) that implements the `osmscout::SymbolRenderer` interface using Skia drawing primitives
2. Implementing `MapPainterSkia::DrawSymbol` using `SymbolRendererSkia`
3. Implementing `MapPainterSkia::DrawContourSymbol` with path-following logic (analogous to Qt's approach using `FollowPath`)
4. Implementing `MapPainterSkia::DrawIcon` using Skia image loading from PNG files
5. Adding a `TODO.md` in the skia backend top-level directory documenting all still-missing features

## Capabilities

### New Capabilities

- `symbol-renderer-skia`: New `SymbolRendererSkia` class in `libosmscout-map-skia` that implements `osmscout::SymbolRenderer`. Provides `SetFill`, `SetBorder`, `BeginPrimitive`, `DrawPolygon`, `DrawRect`, `DrawCircle`, and `EndPrimitive` using Skia `SkPaint`, `SkPath`, and `SkCanvas` APIs. Follows the same pattern as `SymbolRendererCairo` (fill-then-border with dash support) and `SymbolRendererQt`.

- `draw-symbol`: Implement `MapPainterSkia::DrawSymbol` to instantiate `SymbolRendererSkia` and call `renderer.Render(projection, symbol, screenPos, scaleFactor)`, matching the pattern in `MapPainterCairo::DrawSymbol` and `MapPainterQt::DrawSymbol`.

- `draw-contour-symbol`: Implement `MapPainterSkia::DrawContourSymbol` to place symbols along a path. Uses path-following logic (similar to Qt's `FollowPath`/`FollowPathInit` approach) to walk the contour and place symbols at intervals with rotation matching the path tangent. Uses `SymbolRendererSkia` with `afterRenderTransformer`/`afterEndTransformer` callbacks (similar to Cairo's `MapPathOnPath` approach) to transform symbol geometry onto the contour.

- `draw-icon`: Implement `MapPainterSkia::DrawIcon` to load PNG icon images from the configured icon paths, cache them as `sk_sp<SkImage>`, and draw them scaled to the requested dimensions. Follows the pattern from `MapPainterCairo::DrawIcon` and `MapPainterQt::DrawIcon`.

- `skia-todo-manifest`: Add `TODO.md` at `libosmscout-map-skia/TODO.md` documenting all still-missing features compared to the Cairo and Qt backends, including: pattern support in `HasIcon`, SVG icon loading, `StyleSheetChanged` cleanup, `HasPattern` support, and any other gaps identified.

### Modified Capabilities

- `map-painter-skia`: Update `MapPainterSkia` header to declare `SymbolRendererSkia` as a friend class (if needed for label layouter access), add icon image cache (`std::map<std::string, sk_sp<SkImage>>`), and add `FollowPath`/`FollowPathInit` helper methods for contour symbol placement.

## Impact

- **New files**: `libosmscout-map-skia/include/osmscoutmapskia/SymbolRendererSkia.h`, `libosmscout-map-skia/src/osmscoutmapskia/SymbolRendererSkia.cpp`
- **Modified files**: `libosmscout-map-skia/include/osmscoutmapskia/MapPainterSkia.h`, `libosmscout-map-skia/src/osmscoutmapskia/MapPainterSkia.cpp`, `libosmscout-map-skia/CMakeLists.txt`, `libosmscout-map-skia/src/meson.build`, `libosmscout-map-skia/include/meson.build`
- **New file (non-code)**: `libosmscout-map-skia/TODO.md`
- **Dependencies**: No new external dependencies. Uses existing Skia APIs (`SkImage`, `SkPaint`, `SkPath`, `SkCanvas`, `SkData`).
- **API**: No public API changes. `SymbolRendererSkia` is internal to the skia backend (like `SymbolRendererCairo`).
