## Purpose

Render shield labels (path shields with background rectangle, border, and text) consistently across map backends so the label text is visually centered inside the shield background and border, with consistent padding on all sides.

## ADDED Requirements

### Requirement: Shield text is centered within the shield

All backends SHALL draw the text of a shield label so that the drawn text is equidistant (within a small tolerance) from the shield background rectangle's horizontal edges as well as its vertical edges, i.e. the text is centered within the shield background.

#### Scenario: Text horizontally centered in shield
- **GIVEN** a backend renderer (Cairo with Pango, Cairo without Pango, Qt, Skia, SVG, AGG) and a shield label for a non-empty text
- **WHEN** the shield is rendered at a label position with known geometry
- **THEN** the horizontal distance from the left edge of the drawn text bounding box to the shield background's left edge and the distance from the text's right edge to the background's right edge SHALL be equal within 2 pixels

#### Scenario: Text vertically centered in shield
- **GIVEN** a backend renderer (Cairo with Pango, Cairo without Pango, Qt, Skia, SVG, AGG) and a shield label for a non-empty text
- **WHEN** the shield is rendered at a label position with known geometry
- **THEN** the vertical distance from the top edge of the drawn text bounding box to the shield background's top edge and the distance from the text's bottom edge to the background's bottom edge SHALL be equal within 2 pixels

### Requirement: Consistent shield geometry across backends

For the same text, font, font size, and shield style, all backends SHALL produce shields of the same size (within tolerance) around the text: the background rectangle, the border rectangle inset, and the padding between text and background SHALL use the same margins in every backend.

#### Scenario: Shield baseline rectangle is derived from measured label
- **GIVEN** the same text, font, font size, and shield style measured by every backend
- **WHEN** each backend computes the shield geometry
- **THEN** every backend SHALL derive the shield from the backend's measured label rectangle, and the measured label dimensions SHALL be backend-equal per the text measurement contract

#### Scenario: Identical padding across backends
- **GIVEN** the same text, font, font size, and shield style
- **WHEN** each backend renders the shield
- **THEN** the padding between the text bounding box and the shield border SHALL be identical across the Cairo, Qt, Skia, SVG, and AGG backends within 1 pixel

### Requirement: Shield border drawn inside the background

All backends SHALL draw the shield border completely inside the shield background rectangle, and the shield background SHALL be non-smaller than the border rectangle.

#### Scenario: Border does not exceed background
- **GIVEN** a shield label rendered by any backend
- **WHEN** the background and border rectangles are evaluated
- **THEN** the border rectangle SHALL lie fully within the background rectangle