## 1. Font management

- [x] 1.1 Add `FontDescriptor` struct and font cache (`std::map<FontDescriptor, sk_sp<SkTypeface>>`) to `MapPainterSkia`
- [x] 1.2 Implement private `GetFont()` method: load typeface via `SkFontMgr::legacyMakeTypeface()`, cache by descriptor, fall back to default on failure
- [x] 1.3 Implement `GetFontHeight()` using `SkFontMetrics::fDescent - fAscent` instead of hardcoded 12.0

## 2. Label layout

- [x] 2.1 Implement `Layout()`: create `SkFont` from cached typeface, measure text via `measureText()`, get metrics via `SkFontMetrics`, populate `SkiaLabel` with width/height/text
- [x] 2.2 Implement word wrapping in `Layout()`: split on spaces when `enableWrapping == true` and `objectWidth > 0`, measure each line, stack vertically

## 3. Label rendering

- [x] 3.1 Implement `DrawLabel()` for normal style: `drawString()` with text color and alpha at label position
- [x] 3.2 Implement `DrawLabel()` for emphasize style: draw text offset 4 times (1px N/S/E/W) in emphasize color, then draw in text color on top
- [x] 3.3 Implement `DrawLabel()` for shield style: fill background rect, stroke border rect, draw text on top

## 4. Glyph rendering

- [x] 4.1 Implement `DrawGlyphs()`: for each glyph, save canvas, translate to glyph position, rotate by glyph angle, draw glyph text via `drawString()`, restore canvas
- [x] 4.2 Implement `GlyphBoundingBox()`: measure single glyph via `SkFont::measureText()`, return width and height

## 5. Label registration and drawing pipeline

- [x] 5.1 Implement `RegisterRegularLabel()`: forward to `labelLayouter.RegisterLabel()`
- [x] 5.2 Implement `RegisterContourLabel()`: forward to `labelLayouter.RegisterContourLabel()`
- [x] 5.3 Implement `DrawLabels()`: call `labelLayouter.Layout()`, `labelLayouter.DrawLabels()`, `labelLayouter.Reset()`

## 6. Build and test verification

- [x] 6.1 Build with CMake and verify no errors
- [x] 6.2 Build with Meson and verify no errors
- [x] 6.3 Run full test suite and verify no regressions
- [x] 6.4 Add unit tests for font loading, label layout, label rendering, and glyph rendering

## 7. Post-review fixes

- [x] 7.1 Store typeface in `SkiaNativeLabel` during `Layout()` and use it in `DrawLabel()` instead of re-looking-up by name
- [x] 7.2 Move font creation outside per-glyph loop in `DrawGlyphs()` for performance
- [x] 7.3 Add text rendering unit tests (font construction, measureText, canvas drawString, canvas transform, glyph/label data structures)
