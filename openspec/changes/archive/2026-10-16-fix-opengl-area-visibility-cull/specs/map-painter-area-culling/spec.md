## ADDED Requirements

### Requirement: Every painter applies a stylesheet-derived tolerance in the frame's pixels

A painter that prepares the areas of a frame SHALL apply a visibility tolerance it derives from a
width a stylesheet declares as a screen-space length of the frame, whatever its preparation looks
like - whether it shares the core painter's preparation or owns its own. A decision reached from such
a width SHALL convert it with the frame's projection before it uses it, so the tolerance grows with
the DPI of the frame.

#### Scenario: A painter that owns its decision keeps the border of an area crossing the edge

- **GIVEN** a painter that derives the tolerance of its ring visibility decision from the border width the loaded stylesheet declares, and a frame whose projection converts half of that border width to more than one pixel
- **WHEN** an area is placed so that its bounding box reaches the view by less than that converted half-width
- **THEN** the frame SHALL contain the border pixels of that area
- **AND** the same view rendered by the same build without the conversion SHALL NOT contain them

#### Scenario: The tolerance of such a painter follows the DPI of the frame

- **GIVEN** the same view and stylesheet prepared by the same painter at two DPIs
- **WHEN** the rings that reach per-ring preparation are compared
- **THEN** the preparation at the higher DPI SHALL keep every ring whose bounding box reaches the view by less than that frame's converted half-width
- **AND** it SHALL keep at least every ring the preparation at the lower DPI keeps

#### Scenario: An area beyond the converted tolerance still contributes nothing

- **GIVEN** a frame whose projection converts half of a declared border width to more than one pixel, and an area larger than the smallest dimension the draw parameters draw
- **WHEN** that area is placed so that its bounding box lies outside the view by more than the converted half-width
- **THEN** that area SHALL contribute no prepared geometry to the frame
