## Context

The Skia map backend (`libosmscout-map-skia`) was recently added with core rendering capabilities: area fills, path drawing, label layout, and text rendering. However, three pure-virtual methods from `MapPainter` remain stubs:

- `DrawSymbol` — `// no-op: symbol rendering not yet implemented`
- `DrawContourSymbol` — `// no-op: contour symbol rendering not yet implemented`
- `DrawIcon` — `// no-op: icon rendering not yet implemented`
- `HasIcon` — `return false`

The Cairo and Qt backends each have a `SymbolRenderer` subclass (`SymbolRendererCairo`, `SymbolRendererQt`) that implements the `osmscout::SymbolRenderer` interface. The base `SymbolRenderer::Render()` method handles coordinate transformation, projection scaling, and primitive iteration; each backend only needs to implement 6 protected methods: `SetFill`, `SetBorder`, `BeginPrimitive`, `DrawPolygon`, `DrawRect`, `DrawCircle`, `EndPrimitive`.

For contour symbols, Cairo uses `cairo_copy_path_flat` + `MapPathOnPath` with `afterRenderTransformer`/`afterEndTransformer` callbacks. Qt uses `FollowPath`/`FollowPathInit` with `QTransform` rotation. Both approaches walk the path and place symbols at intervals with tangent-aligned rotation.

For icons, Cairo loads PNGs via `cairo_image_surface_create_from_png` and caches them as `cairo_surface_t*`. Qt loads PNGs via `QImage` and caches in a `std::map<std::string, QImage>`. Both scale the image to the requested icon dimensions.

## Goals / Non-Goals

**Goals:**
- Implement `SymbolRendererSkia` — a `SymbolRenderer` subclass using Skia `SkPaint`/`SkPath`/`SkCanvas` APIs
- Implement `MapPainterSkia::DrawSymbol` using `SymbolRendererSkia`
- Implement `MapPainterSkia::DrawContourSymbol` with path-following and tangent-aligned symbol placement
- Implement `MapPainterSkia::DrawIcon` with PNG loading, caching, and scaled rendering
- Implement `MapPainterSkia::HasIcon` with icon path lookup and dimension setup
- Add `TODO.md` documenting all remaining gaps vs Cairo/Qt backends
- Update build files (CMakeLists.txt, meson.build) for new source/header files

**Non-Goals:**
- SVG icon loading (Cairo also has `// TODO: add support for reading svg images`)
- Pattern support in symbols (Cairo and Qt both warn "Pattern is not supported for symbols")
- `StyleSheetChanged` cleanup for icon/pattern caches (can be added later)
- Performance optimization beyond basic caching

## Decisions

### Decision 1: SymbolRendererSkia as standalone class (not inline in MapPainterSkia)

**Chosen:** Standalone `SymbolRendererSkia` class in separate files.

**Alternatives considered:**

1. **Standalone class** (chosen) — Matches Cairo/Qt/iOS/SVG patterns. Clean separation of concerns. `SymbolRendererSkia` only needs an `SkCanvas*` and manages its own `SkPaint` state. Easy to test independently (see `Tests/src/SymbolRendererSVGTest.cpp` for precedent).

2. **Inline in MapPainterSkia** — Would require adding 6 protected methods to `MapPainterSkia` directly. Violates existing backend convention. `MapPainterSkia` is already 750+ lines; adding symbol primitive methods would bloat it further.

**Rationale:** Consistency with all other backends. The `SymbolRenderer` interface is designed for this pattern — each backend provides a thin wrapper over its graphics API.

### Decision 2: Contour symbol path-following — Qt-style FollowPath approach

**Chosen:** Qt-style `FollowPath`/`FollowPathInit` approach adapted to Skia.

**Alternatives considered:**

1. **Qt-style FollowPath** (chosen) — Walks the coordinate range directly using `CoordBufferRange` accessors. No intermediate path representation needed. Gives precise control over position and tangent calculation. Used by Qt backend successfully.

2. **Cairo-style MapPathOnPath** — Requires flattening the path to line segments via `SkPath::Iter` or `SkPath::RawIter`, then mapping transformed geometry. More complex and fragile. Cairo's approach is specific to cairo's path API.

3. **Skia path measurement** — `SkPath::computeTightBounds()` and manual segment walking. Skia lacks a built-in "walk path by distance" utility, so we'd need to implement it ourselves anyway.

**Rationale:** The Qt approach operates directly on the coordinate buffer, which is simpler and more portable. We'll add `FollowPathInit` and `FollowPath` helper methods to `MapPainterSkia` (private), matching Qt's pattern. The symbol is drawn at each position with a rotation transform applied to the canvas.

### Decision 3: Icon image storage — `sk_sp<SkImage>` with `std::map`

**Chosen:** `std::map<std::string, sk_sp<SkImage>>` keyed by icon name.

**Alternatives considered:**

1. **`std::map<std::string, sk_sp<SkImage>>`** (chosen) — Qt uses icon name as key. Cairo uses numeric index. Skia has no built-in image registry, so name-based lookup is simpler. `SkImage` supports scaling via `SkCanvas::drawImageRect`.

2. **Numeric index (Cairo-style)** — Requires resizing a vector and tracking IDs. Cairo's `IconStyle::GetIconId()` returns a 1-based index, but this is Cairo-specific. The base `IconStyle` uses `GetIconName()`.

3. **`SkBitmap`** — `SkImage` is preferred for rendering; `SkBitmap` is for pixel manipulation. `SkImages::DeferredFromEncodedData` handles PNG decoding directly.

**Rationale:** Name-based key matches Qt's approach and avoids coupling to Cairo's ID scheme. `SkImage` is the idiomatic Skia type for rendering.

### Decision 4: HasIcon dimension setup — match Cairo's pattern

**Chosen:** Follow Cairo's `HasIcon` logic for dimension setup based on `IconMode`.

**Rationale:** Cairo handles three icon modes:
- `Scalable`/`ScaledPixmap` — use `parameter.GetIconSize()` converted to pixels
- `OriginalPixmap` — use actual image dimensions after loading

Qt always uses `parameter.GetIconSize()`. Cairo's approach is more complete. We'll match it.

## Risks / Trade-offs

- **[Risk] Icon loading from disk on draw thread** → Mitigation: Cache loaded icons in `std::map`. First access per icon loads from disk; subsequent accesses are O(1) map lookup. Same pattern as Cairo and Qt.
- **[Risk] Contour symbol rotation accuracy** → Mitigation: Use 3-point tangent calculation (Qt's approach: advance half-width, sample midpoint, advance half-width, compute atan2). This gives smooth rotation even on curved paths.
- **[Risk] Missing `StyleSheetChanged` cleanup** → Mitigation: Icons and patterns are cached but never freed on style change. This is a known gap shared with the initial skia implementation. Document in `TODO.md`.
- **[Risk] Pattern fill in symbols not supported** → Mitigation: Match Cairo/Qt behavior — log warning and skip pattern fills in `SymbolRendererSkia::SetFill`. This is a known limitation across all backends.
