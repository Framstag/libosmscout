## Purpose

Defines the contract for the map painter's area preparation step - the work that turns the
areas loaded into a view into the prepared areas of a frame. It exists so that the cost of
that step follows the areas a frame actually draws rather than the areas the viewport
happens to load, while the prepared area set, the draw order and the rendered output stay
exactly as they are.

## ADDED Requirements

### Requirement: Area preparation does not allocate per loaded area

Preparing the areas of a frame SHALL NOT perform heap allocation that grows with the number
of areas loaded into the view; the allocation count of the step SHALL instead be bounded by a
constant independent of how many areas are loaded.

#### Scenario: Repeated rendering of a loaded view stays within a constant allocation bound

- **GIVEN** a view whose loaded areas greatly outnumber the areas that are prepared for drawing
- **WHEN** the same view is rendered repeatedly
- **THEN** the area preparation step SHALL perform at most a small constant number of heap allocations per frame, independent of the number of loaded areas

#### Scenario: A view with more loaded areas does not raise the allocation count of the step

- **GIVEN** two views that prepare a comparable number of areas but load very different numbers of areas
- **WHEN** both views are rendered
- **THEN** the area preparation step of the heavier view SHALL NOT perform substantially more heap allocations than that of the lighter view

### Requirement: Styling and visibility are decided before geometry is transformed

For every ring of a loaded area, the painter SHALL decide whether the ring is styled and
whether it is visible before transforming that ring's geometry into the frame's coordinate
data, and SHALL NOT transform the geometry of a ring that fails either decision.

#### Scenario: Rings outside the viewport are not transformed

- **GIVEN** a loaded area whose rings lie outside the current viewport
- **WHEN** a frame is prepared
- **THEN** none of the rings of that area SHALL contribute transformed coordinates to the frame

#### Scenario: Unstyled rings are not transformed

- **GIVEN** a loaded area whose ring resolves neither a fill style nor any border style
- **WHEN** a frame is prepared
- **THEN** that ring SHALL NOT contribute transformed coordinates to the frame

#### Scenario: Styled and visible rings are transformed as before

- **GIVEN** an area whose rings are styled in the current stylesheet and visible in the current viewport
- **WHEN** a frame is prepared
- **THEN** each of those rings SHALL contribute the same transformed coordinates to the frame as before this change
- **THEN** the indices of the frame's coordinate buffer MAY differ from before, because the buffer holds only the rings that take part in the frame

### Requirement: Clipping rings keep valid geometry

A prepared area SHALL keep valid coordinate ranges for the rings that serve as clipping
regions of a drawn ring, even when those rings are not drawn themselves.

#### Scenario: Holes of a drawn area are still clipped

- **GIVEN** a drawn outer ring with an inner ring marked as ignored, which serves as a clipping region
- **WHEN** the frame is prepared and rendered
- **THEN** the prepared area SHALL reference a valid coordinate range for that clipping ring
- **THEN** the rendered output SHALL show the same clipping as before this change

#### Scenario: Clipping rings that are not styled are still available

- **GIVEN** a clipping ring that resolves no fill style and no border style
- **WHEN** its parent ring is prepared
- **THEN** the clipping ring SHALL still contribute a valid coordinate range to the parent's prepared entry

### Requirement: Prepared areas, draw order and rendered output are unchanged

For any view, the painter SHALL prepare the same set of areas and rings, in the same
preparation order, and SHALL produce the same rendered output as before this change,
including for areas whose ordering criteria compare equal.

#### Scenario: The number of prepared areas is unchanged

- **GIVEN** a view whose prepared area count is known from before the change
- **WHEN** that view is prepared after the change
- **THEN** the number of prepared areas SHALL be identical to the known count

#### Scenario: Draw order of prepared areas is unchanged

- **GIVEN** a view that prepares several areas, including areas whose ordering criteria compare equal
- **WHEN** the prepared areas are enumerated in draw order
- **THEN** the enumeration SHALL be identical to the enumeration of the same view before the change, including for the equal-comparing areas

#### Scenario: Rendered output is unchanged

- **GIVEN** a view rendered before the change
- **WHEN** the same view is rendered after the change, with the same stylesheet and parameters
- **THEN** the rendered output SHALL be identical
