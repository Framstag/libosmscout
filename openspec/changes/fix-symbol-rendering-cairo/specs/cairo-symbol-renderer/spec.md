## Purpose

Lets renderers and tools that use the Cairo backend rely on symbol drawing behaving consistently: border widths scale with the symbol canvas, polygon outlines are closed, fills and strokes render, and dashed borders scale. This capability documents that contract so it can be verified by automated tests.

## ADDED Requirements

### Requirement: Border widths scale from mm to pixels
The system SHALL render symbol border widths scaled from millimeters to the symbol's pixel coordinate space, so that a border defined in mm has the resulting pixel width given by the product of the mm width and the current mm-per-pixel factor.

#### Scenario: Border width converted by mm-per-pixel factor
- **GIVEN** a symbol border defined with width `1.0` and a mm-per-pixel factor of `2.0`
- **WHEN** the symbol is rendered
- **THEN** the stroked border SHALL occupy a band of `2` pixels (sampled pixels at the expected offset SHALL show the border color)

#### Scenario: Sub-pixel border width conversion
- **GIVEN** a symbol border defined with width `0.5` and a mm-per-pixel factor of `2.0`
- **WHEN** the symbol is rendered
- **THEN** the stroked border SHALL occupy a band of `1` pixel

### Requirement: Polygon outlines are closed
The system SHALL render stroked polygons as closed shapes, so the outline includes the segment from the last vertex back to the first.

#### Scenario: Stroked triangle includes closing edge
- **GIVEN** a triangle primitive with three vertices and a visible border
- **WHEN** the symbol is rendered
- **THEN** pixels along the edge between the last and first vertex SHALL show the border color

### Requirement: Fills and strokes are emitted
The system SHALL render symbols with the configured fill color, border color, or both, and SHALL emit nothing visible for a primitive with neither fill nor border.

#### Scenario: Filled and stroked shape
- **GIVEN** a shape with visible fill color and visible border color
- **WHEN** the symbol is rendered
- **THEN** interior pixels SHALL show the fill color and border pixels SHALL show the border color

#### Scenario: Shape without fill or border
- **GIVEN** a shape with no fill and no border (or fully transparent colors)
- **WHEN** the symbol is rendered
- **THEN** the rendered region SHALL show the background color

### Requirement: Dashed borders scale with border width
The system SHALL render dashed borders so that dash and gap lengths scale with the converted border width.

#### Scenario: Dash pattern scaled
- **GIVEN** a dashed border whose dash values are defined relative to border width
- **WHEN** the symbol is rendered
- **THEN** the rendered dashes and gaps SHALL be proportional to the converted border width (gap pixels SHALL show the background color between dash pixels)
