## What Changes

Add missing line and area border style features to the Skia map backend (`libosmscout-map-skia`), matching capabilities already present in the Cairo and Qt backends.

The Skia backend currently supports basic solid-color strokes for paths and area borders, but lacks: gap color for dashed lines, proper cap style handling, border dash patterns, alpha/transparency, area clippings (holes), and pattern fills. These are all used by standard OSM stylesheets.

## Capabilities

### `line-gap-color`
Render gap color behind dashed lines. When a `LineStyle` has dashes and a visible `gapColor`, the backend draws a solid line in the gap color first, then the dashed line on top. Cairo and Qt both implement this pattern in `DrawWay()` → two `DrawPath()` calls.

### `line-cap-styles`
Honor `LineStyle::CapStyle` (butt, round, square) for both `joinCap` and `endCap`. Skia currently hardcodes `kRound_Cap`. Must map `capButt` → `SkPaint::kButt_Cap`, `capRound` → `kRound_Cap`, `capSquare` → `kSquare_Cap`.

### `line-alpha-transparency`
Use RGBA color (via `SkColorSetARGB`) instead of RGB-only (`SkColorSetRGB`). Affects `DrawPath()`, `DrawArea()` fill, and `DrawArea()` border. Cairo and Qt both pass alpha through.

### `border-dash-patterns`
Apply dash patterns from `BorderStyle::GetDash()` when drawing area borders in `DrawArea()`. Currently border is always solid.

### `border-gap-color`
Render gap color for dashed area borders, analogous to line gap color. When `BorderStyle` has dashes and visible `gapColor`, draw solid gap color first, then dashed border on top.

### `area-clippings`
Support `AreaData::clippings` (interior holes) using even-odd fill rule via `SkPath::setFillType(SkPathFillType::kEvenOdd)`. Cairo uses `CAIRO_FILL_RULE_EVEN_ODD`; Qt adds sub-paths.

### `pattern-fills`
Implement `DrawFillStyle()` stub to support pattern fills and borders for areas. Load PNG pattern images, create `SkShader` with `SkTileMode::kRepeat`, and apply as fill paint shader. Also handle the fill+border combined case (fill then stroke).

## Impact

- **`libosmscout-map-skia/src/osmscoutmapskia/MapPainterSkia.cpp`** — Main implementation changes
- **`libosmscout-map-skia/include/osmscoutmapskia/MapPainterSkia.h`** — May need new private helpers (e.g., `SetLineAttributes`, pattern cache)
- **`libosmscout-map-skia/CMakeLists.txt`** — No changes expected (Skia already linked)
- **`libosmscout-map-skia/meson.build`** — No changes expected
- **`Tests/`** — Visual comparison tests could be added but not required for this change
