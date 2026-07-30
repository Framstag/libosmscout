## Context

The Skia backend (`libosmscout-map-skia`) has all text rendering methods as no-ops:

| Method | Current state |
|--------|--------------|
| `GetFontHeight()` | Returns hardcoded 12.0 |
| `Layout()` | Returns empty `SkiaLabel` with no text metrics |
| `DrawLabel()` | No-op |
| `DrawGlyphs()` | No-op |
| `GlyphBoundingBox()` | Returns `(0,0,0,0)` |
| `RegisterRegularLabel()` | No-op |
| `RegisterContourLabel()` | No-op |
| `DrawLabels()` | No-op |
| `HasIcon()` | Returns false |

The Cairo backend uses Pango (with cairo fallback) for full text layout, glyph extraction, and rendering with normal/emphasize/shield styles. The Qt backend uses `QTextLayout`/`QPainter`. Both support path-following contour labels via per-glyph positioning.

Skia provides `SkFont`, `SkFontMgr`, `SkTypeface`, and `SkTextBlob` for text rendering. This design uses `SkFont` directly (matching the simpler AGG backend approach) rather than a complex layout library.

## Goals / Non-Goals

**Goals:**

- Implement all 7 text rendering capabilities from the proposal
- Match visual output of Cairo/Qt for normal, emphasize, and shield label styles
- Support path-following (contour) labels via per-glyph transforms
- Use Skia's built-in font APIs — no external text layout library

**Non-Goals:**

- No complex text shaping (Skia handles basic Latin/ASCII well; RTL/CTL not required)
- No icon rendering (separate capability, `HasIcon()` stays as-is)
- No symbol rendering (separate capability)
- No contour symbol rendering (separate capability)

## Decisions

### Decision 1: Font cache using `std::map<FontDescriptor, sk_sp<SkTypeface>>`

**Chosen:** Cache `SkTypeface` objects keyed by `(fontName, fontSize)` using a struct `FontDescriptor` as map key. Use `SkFontMgr::legacyMakeTypeface()` to load system fonts.

**Rationale:** Cairo caches `PangoFontDescription*` by font size; Qt caches `QFont` by descriptor. Skia's `SkTypeface` is the immutable typeface object. `SkFont` is lightweight and wraps a typeface + size, so caching typefaces is sufficient. `SkFont` objects can be created on the fly from cached typefaces.

**Alternatives considered:**
- Cache `SkFont` objects directly — rejected because `SkFont` is cheap to construct from a typeface; caching typefaces is more flexible
- Load fonts from file paths — rejected because system fonts via `SkFontMgr` are sufficient and match Cairo/Qt behavior

### Decision 2: Label layout using `SkFont::measureText()`

**Chosen:** Implement `Layout()` by measuring text width via `SkFont::measureText()` and height via `SkFontMetrics`. Support word wrapping by splitting on spaces when `enableWrapping` is true.

**Rationale:** Cairo's non-Pango fallback uses `cairo_text_extents_t` and `cairo_font_extents_t`. Skia's `measureText()` and `SkFontMetrics` provide equivalent information. Word wrapping is done manually (matching Cairo's non-Pango path and the AGG backend).

**Alternatives considered:**
- Use `SkTextBlob` for layout — rejected because `SkTextBlob` is for rendering, not measurement
- Use a full text layout library (Pango, HarfBuzz) — rejected to avoid new dependencies; Cairo's Pango path is optional

### Decision 3: Glyph rendering via per-character `SkFont` + transform

**Chosen:** In `DrawGlyphs()`, for each glyph, set up a temporary `SkFont` with the glyph's font, apply translation and rotation transforms on the canvas, and draw the glyph text using `SkCanvas::drawString()`.

**Rationale:** Cairo's Pango path extracts individual glyphs from `PangoLayout` and renders each with `pango_cairo_show_glyph_string()` after applying rotation. The non-Pango fallback uses `cairo_show_text()` with transforms. Skia's `drawString()` with canvas transforms achieves the same result.

**Alternatives considered:**
- Use `SkTextBlob::MakeFromRSXform()` for batched glyph rendering — rejected because glyphs come one at a time from the label layouter; batching would require collecting all glyphs first
- Use `SkCanvas::drawGlyphs()` — not available in this Skia version

### Decision 4: Label rendering with three style paths

**Chosen:** Implement `DrawLabel()` with three branches matching Cairo:
- **normal**: `drawString()` with text color
- **emphasize**: `drawString()` with outline color at multiple offsets (simulating stroke), then fill color on top
- **shield**: Fill background rectangle, draw border rectangle, then draw text on top

**Rationale:** Cairo uses the exact same approach for all three styles. Qt uses `QPainter::drawText()` with similar logic for emphasize (draws text offset 4 times for outline effect). The shield style matches Cairo's `cairo_rectangle()` + `cairo_fill()` + `cairo_stroke()` pattern.

**Alternatives considered:**
- Use `SkPaint::setStyle(kStrokeAndFill_Style)` for emphasize — rejected because Skia doesn't support simultaneous stroke+fill on text; the offset approach matches Cairo/Qt
- Use `SkTextBlob` for shield background — rejected because simple rectangles are sufficient

## Risks / Trade-offs

- **Font availability** → If the requested font name is not available on the system, `SkFontMgr::legacyMakeTypeface()` returns nullptr. Mitigation: fall back to a default typeface (Skia's built-in) and log a warning, matching Cairo behavior.
- **Text encoding** → Skia's `drawString()` accepts `const char[]` (UTF-8). The Cairo non-Pango path uses `cairo_show_text()` which also takes UTF-8. No encoding issues expected for Latin/ASCII text.
- **Multi-line labels** → Word wrapping is done manually by splitting on spaces. This matches Cairo's non-Pango path. Pango-based Cairo does proper word wrapping with `pango_layout_set_width()`. The manual approach may produce different line breaks for non-Latin text.
- **Performance** → Per-glyph transforms in `DrawGlyphs()` involve individual canvas save/restore per glyph. For long contour labels this could be slow. Mitigation: same approach as Cairo/Qt; acceptable for typical OSM label density.
