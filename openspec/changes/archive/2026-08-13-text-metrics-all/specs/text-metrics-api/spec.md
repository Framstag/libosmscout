# Text Metrics API

**Purpose:** Provide a public measurement API on the map painter base class so external tools can obtain label dimensions and per-glyph bounding boxes for a given text, font, and font size, consistently across all rendering backends.

## ADDED Requirements

### Requirement: Measure text metrics

The map painter base class SHALL expose a public method that, given a text, a font name, a font size, and rendering parameters, returns the label dimensions and the per-glyph bounding boxes for that text.

#### Scenario: Measurement returns label dimensions
- **GIVEN** a painter instance and a non-empty text with a valid font and font size
- **WHEN** the measurement method is called
- **THEN** it SHALL return a label width and height greater than zero

#### Scenario: Measurement returns one entry per glyph
- **GIVEN** a painter instance and a text with N characters
- **WHEN** the measurement method is called
- **THEN** it SHALL return exactly N glyph entries, one per character

#### Scenario: Glyph box is relative to glyph base point
- **GIVEN** a painter instance and a text
- **WHEN** the measurement method is called
- **THEN** each glyph entry SHALL include a bounding box whose coordinates are relative to the glyph's base point (the left baseline origin)

#### Scenario: Glyph position is relative to label origin
- **GIVEN** a painter instance and a text
- **WHEN** the measurement method is called
- **THEN** each glyph entry SHALL include a position relative to the label origin

### Requirement: Consistent measurement across backends

All Linux backends (Cairo, AGG, Qt, Skia, SVG) SHALL implement the measurement method such that for the same text, font, font size, and rendering parameters, the returned label dimensions and glyph positions are equal within a small tolerance.

#### Scenario: Same text measured by all backends
- **GIVEN** the same text, font file, font size, and rendering parameters
- **WHEN** the measurement method is called on each of the Cairo, AGG, Qt, Skia, and SVG painters
- **THEN** the returned label widths SHALL be equal within a tolerance of 0.5 pixels
- **AND** the returned glyph positions SHALL be equal within a tolerance of 0.5 pixels

### Requirement: Glyph bounding box matches drawn ink

The per-glyph bounding box returned by the measurement method SHALL enclose the ink actually drawn for that glyph.

#### Scenario: Box encloses drawn glyph
- **GIVEN** a painter instance and a text
- **WHEN** the text is drawn at the glyph base points and the returned bounding boxes are overlaid
- **THEN** every drawn pixel of each glyph SHALL lie inside its bounding box

#### Scenario: Box is tight around drawn glyph
- **GIVEN** a painter instance and a text
- **WHEN** the text is drawn at the glyph base points and the returned bounding boxes are overlaid
- **THEN** each bounding box SHALL have at least one drawn pixel touching each of its four edges
