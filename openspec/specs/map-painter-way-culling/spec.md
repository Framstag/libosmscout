# map-painter-way-culling Specification

## Purpose

Defines the contract for the map painter's early rejection of ways that cannot be visible in
a view. It exists so that the cost of way preparation and of way shield registration follows
the ways a frame can actually draw rather than the ways the viewport happens to load, while
the prepared ways, the draw order and the rendered output stay exactly as they are.

## Requirements

### Requirement: A way that cannot be visible is rejected before its preparation

For every loaded way, the painter SHALL decide whether the way can contribute a pixel to the
current view before resolving the way's line styles, before registering the way's shield
labels and before transforming the way's geometry into the frame's coordinate data, and SHALL
NOT perform any of that work for a rejected way.

The decision SHALL be conservative: it SHALL NOT reject a way that the existing per-line-style
visibility decision would have kept, for the current stylesheet, zoom level and map
parameters.

#### Scenario: A way outside the view contributes nothing

- **GIVEN** a loaded way whose bounding box lies outside the current viewport by more than any drawn line style of the current level can reach
- **WHEN** a frame is prepared
- **THEN** that way SHALL NOT appear among the prepared ways of the frame
- **THEN** no of its points SHALL contribute transformed coordinates to the frame
- **THEN** no shield label SHALL be registered for that way

#### Scenario: A way inside the view is prepared as before

- **GIVEN** a way that is styled in the current stylesheet and visible in the current viewport
- **WHEN** a frame is prepared
- **THEN** the way SHALL appear among the prepared ways of the frame with the same line styles, the same transformed coordinates and the same draw order position as before this change

#### Scenario: The rejection is never more aggressive than the per-line-style decision

- **GIVEN** two stylesheets whose widest way line style differs by a large factor
- **WHEN** the same view is prepared with each of them
- **THEN** the reach of the early rejection SHALL grow with the stylesheet, and SHALL never be smaller than the widest line width the per-line-style visibility decision can use at that level
- **THEN** a way that lies inside the viewport plus that widest line width SHALL still be prepared

### Requirement: Way preparation work follows the visible ways, not the loaded ways

The work and the heap allocation of way preparation SHALL be bounded by the ways that can be
visible in the view, and SHALL NOT grow with the number of ways that are loaded into the view
but cannot contribute a pixel.

#### Scenario: Additional loaded ways outside the view do not add work

- **GIVEN** a view and the same view with a large number of further loaded ways whose bounding boxes lie outside the viewport
- **WHEN** both views are prepared repeatedly
- **THEN** the prepared ways, their transformed coordinates, their draw order and the registered label set SHALL be identical between the two views
- **THEN** the way preparation step SHALL NOT perform substantially more heap allocations for the view with the additional loaded ways

#### Scenario: The cost of the step does not follow the loaded way count

- **GIVEN** two views that prepare a comparable number of ways but load very different numbers of ways
- **WHEN** both views are prepared
- **THEN** the work of the way preparation step SHALL be governed by the prepared ways of the view, not by the loaded ones

### Requirement: Shield labels of a rejected way are not registered

A way that the early rejection removes SHALL NOT register shield labels, and SHALL NOT
contribute grid positions for shield placement, so that the label stage of the frame is not
given labels that cannot appear.

#### Scenario: A shield-styled way outside the view adds no labels

- **GIVEN** a loaded way that resolves a shield style in the current stylesheet and whose bounding box lies outside the viewport beyond the reach of the early rejection's margin
- **WHEN** a frame is prepared
- **THEN** the label set of the frame SHALL be identical to the label set of the same view without that way
- **THEN** the label stage SHALL NOT be asked to measure a label for that way

#### Scenario: Shield labels of a visible way are unchanged

- **GIVEN** a shield-styled way that is visible in the current viewport
- **WHEN** a frame is prepared
- **THEN** the shield labels registered for it SHALL have the same text, the same positions, the same priorities and the same draw order as before this change

### Requirement: Prepared ways, draw order and rendered output are unchanged

For any view, the painter SHALL prepare the same ways and way paths, in the same preparation
and draw order, with the same geometry and styling, and SHALL produce the same rendered
output as before this change, including for ways whose ordering criteria compare equal.

#### Scenario: Prepared way entries are unchanged

- **GIVEN** a view rendered before the change
- **WHEN** the same view is prepared after the change
- **THEN** the prepared way entries SHALL be identical by type, line style, transformed coordinates and order, including for equal-comparing ways

#### Scenario: Rendered output is unchanged

- **GIVEN** a view rendered before the change, with a stylesheet that draws way shields and contour labels
- **WHEN** the same view is rendered after the change, with the same stylesheet and parameters
- **THEN** the rendered output SHALL be identical
