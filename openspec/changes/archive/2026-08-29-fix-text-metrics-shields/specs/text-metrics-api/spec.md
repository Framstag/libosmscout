## MODIFIED Requirements

### Requirement: Consistent measurement across backends

All Linux backends (Cairo, AGG, Qt, Skia, SVG) SHALL implement the measurement method such that for the same text, font, font size, and rendering parameters, the returned label dimensions and glyph positions are equal within a small tolerance. This SHALL hold for every text stack variant a backend supports (e.g. the Cairo backend both with and without Pango).

#### Scenario: Same text measured by all backends
- **GIVEN** the same text, font file, font size, and rendering parameters
- **WHEN** the measurement method is called on each of the Cairo, AGG, Qt, Skia, and SVG painters
- **THEN** the returned label widths SHALL be equal within a tolerance of 0.5 pixels
- **AND** the returned glyph positions SHALL be equal within a tolerance of 0.5 pixels

#### Scenario: Same text measured by both Cairo text stacks
- **GIVEN** the same text, font file, font size, and rendering parameters
- **WHEN** the measurement method is called on the Cairo painter built with Pango and on the Cairo painter built without Pango
- **THEN** the returned label widths, label heights, and glyph boxes SHALL be equal within a tolerance of 0.5 pixels

### Requirement: Glyph bounding box matches drawn ink

The per-glyph bounding box returned by the measurement method SHALL enclose the ink actually drawn for that glyph. The box SHALL be derived from the drawn outline of the individual glyph, not from overall font metrics.

#### Scenario: Box encloses drawn glyph
- **GIVEN** a painter instance and a text
- **WHEN** the text is drawn at the glyph base points and the returned bounding boxes are overlaid
- **THEN** every drawn pixel of each glyph SHALL lie inside its bounding box

#### Scenario: Box is tight around drawn glyph
- **GIVEN** a painter instance and a text
- **WHEN** the text is drawn at the glyph base points and the returned bounding boxes are overlaid
- **THEN** each bounding box SHALL have at least one drawn pixel touching each of its four edges

#### Scenario: Boxes differ between glyphs of different ink
- **GIVEN** a painter instance and a text containing glyphs with different ink sizes (e.g. a hyphen and a capital letter with same font)
- **WHEN** the measurement method is called
- **THEN** the returned bounding boxes of different glyphs SHALL NOT be identical

## ADDED Requirements

### Requirement: Label dimensions describe the drawn text extents

The label width and height returned by the measurement method SHALL describe the visual extents of the drawn text (the overall ink of the label), not the typographic font box. The returned label rectangle SHALL be the union of the per-glyph bounding boxes (offset by their positions).

#### Scenario: Label height matches ink of rendered text
- **GIVEN** a painter instance, a text, a font, and a font size
- **WHEN** the measurement method is called and the text is rendered at the returned metrics' origin
- **THEN** the vertical distance between the drawn text's topmost and bottommost pixels SHALL equal the returned label height within a tolerance of 1 pixel

#### Scenario: Label rectangle equals union of glyph boxes
- **GIVEN** a painter instance and a text
- **WHEN** the measurement method is called
- **THEN** the union of all glyph bounding boxes (each offset by its glyph position, interpreted relative to the label origin) SHALL equal the returned label width and height within a tolerance of 1 pixel