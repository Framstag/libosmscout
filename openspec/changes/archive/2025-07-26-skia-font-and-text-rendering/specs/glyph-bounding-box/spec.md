## ADDED Requirements

### Requirement: Glyph bounding box

The Skia backend SHALL implement `GlyphBoundingBox()` to return the bounding rectangle of a single glyph using `SkFont::measureText()`.

#### Scenario: GlyphBoundingBox returns non-zero dimensions
- **WHEN** `GlyphBoundingBox()` is called with a glyph containing a printable character
- **THEN** it SHALL return a `ScreenVectorRectangle` with positive width and height based on the glyph's metrics
