## MODIFIED Requirements

### Requirement: Glyph bounding box
The Skia backend SHALL implement `GlyphBoundingBox()` to return the bounding rectangle of a single glyph using `SkFont::measureText()`.

The Skia backend's glyph bounding box is now part of the shared text measurement contract. The Skia backend SHALL provide per-glyph bounding boxes through the text measurement API, and the boxes SHALL enclose the ink actually drawn for the glyph.

#### Scenario: GlyphBoundingBox returns non-zero dimensions
- **WHEN** `GlyphBoundingBox()` is called with a glyph containing a printable character
- **THEN** it SHALL return a `ScreenVectorRectangle` with positive width and height based on the glyph's metrics

#### Scenario: Skia measurement returns ink-enclosing boxes
- **GIVEN** the Skia painter and a text
- **WHEN** the text measurement API is called
- **THEN** each returned per-glyph bounding box SHALL enclose the ink drawn for that glyph
