# Glyph Rendering

## Purpose

The Skia backend implements `DrawGlyphs()` to render individual glyphs along a path. Each glyph has a position, rotation angle, and associated font.

## Requirements

### Requirement: Path-following glyph rendering

The Skia backend SHALL implement `DrawGlyphs()` to render individual glyphs along a path. Each glyph has a position, rotation angle, and associated font. The backend SHALL apply translation and rotation transforms per glyph.

#### Scenario: Glyph renders at correct position and angle
- **WHEN** `DrawGlyphs()` is called with glyphs having position and angle values
- **THEN** each glyph SHALL be rendered using canvas translation to its position and rotation to its angle

#### Scenario: Glyph uses correct font
- **WHEN** `DrawGlyphs()` is called with glyphs having different font associations
- **THEN** each glyph SHALL be rendered using its associated font
