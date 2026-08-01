## What Changes

Implement font and text rendering in the Skia map backend (`libosmscout-map-skia`), matching capabilities already present in the Cairo and Qt backends.

The Skia backend currently has all text-related methods as no-ops or stubs: `DrawLabel()`, `DrawGlyphs()`, `GlyphBoundingBox()`, `Layout()`, `RegisterRegularLabel()`, `RegisterContourLabel()`, `DrawLabels()`, and `GetFontHeight()`. None render visible text.

## Capabilities

### `font-management`
Load and cache `SkFont` instances keyed by font name and size. Implement `GetFontHeight()` using actual font metrics instead of hardcoded 12.0. Support the font name and size from `MapParameter`.

### `label-layout`
Implement `Layout()` to measure text extents using `SkFont`. Return a `SkiaLabel` with correct width, height, and text content. Support word wrapping for multi-line labels when `enableWrapping` is true and `objectWidth` is set.

### `label-rendering`
Implement `DrawLabel()` to render text labels on the canvas. Support three label styles:
- **normal**: Solid text in the style's text color with alpha
- **emphasize**: Text with an outline (stroke then fill) using emphasize color
- **shield**: Text on a colored background rectangle with a border

### `glyph-rendering`
Implement `DrawGlyphs()` to render individual glyphs along a path (contour labels). Each glyph has a position, angle, and font. Use `SkFont` with rotation transforms to place glyphs along the path.

### `glyph-bounding-box`
Implement `GlyphBoundingBox()` to return the bounding rectangle of a single glyph using `SkFont::measureText()` or equivalent.

### `label-registration`
Implement `RegisterRegularLabel()` and `RegisterContourLabel()` to register labels with the `labelLayouter` for layout and overlap resolution.

### `label-drawing`
Implement `DrawLabels()` to drive the label layouter: call `Layout()`, `DrawLabels()` on the layouter, then `Reset()`. This is the entry point that renders all registered labels.

## Impact

- **`libosmscout-map-skia/src/osmscoutmapskia/MapPainterSkia.cpp`** — All text method implementations
- **`libosmscout-map-skia/include/osmscoutmapskia/MapPainterSkia.h`** — Font cache member, SkiaLabel/SkiaGlyph type updates
- **`libosmscout-map-skia/CMakeLists.txt`** — No changes expected
- **`libosmscout-map-skia/meson.build`** — No changes expected
- **`Tests/src/MapPainterSkiaTest.cpp`** — Add text rendering tests
