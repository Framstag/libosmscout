## Purpose

Defines the contract for the volume of the map painter's label stage: which registered labels
are measured, stored and laid out. It exists so that the allocation and time budget of the
label stage follows the labels a frame can actually place rather than the labels the loaded
objects happen to produce, while the label set, the label placement, the draw order, the
measurement results and the rendered output stay exactly as they are.

## ADDED Requirements

### Requirement: A label that cannot intersect the view is neither measured nor stored nor laid out

The painter SHALL decide whether a label can intersect the current view before measuring that
label, before storing it as part of the frame's label set and before laying it out for
overlap. A label that cannot intersect the view SHALL NOT be measured, stored or laid out.
The rule applies to every source of labels: the labels of a node, the labels of an area, the
shield labels of a way (all of which are registered through the shared label stage) and the
contour labels of a way (which the painter registers for prepared ways, so a way that cannot be
visible is already rejected before its contour labels are produced).

The decision SHALL be conservative with respect to the label rectangle the drawing path uses:
it SHALL NOT discard a label that the existing viewport decision of the label drawing path
would have drawn, for the current stylesheet, zoom level and map parameters.

#### Scenario: A label far outside the view is not measured

- **GIVEN** a registered label whose rectangle cannot intersect the current viewport
- **WHEN** a frame is prepared
- **THEN** the label SHALL NOT be measured
- **THEN** the label SHALL NOT be part of the frame's label set

#### Scenario: A label whose rectangle reaches into the view is kept

- **GIVEN** a label whose anchor position lies outside the viewport but whose rectangle intersects the viewport, for example a long label at the viewport edge
- **WHEN** a frame is prepared and rendered
- **THEN** the label SHALL be measured, placed and drawn exactly as before this change

#### Scenario: The same rule holds for every label source

- **GIVEN** a node label, an area label, a way shield label and a contour label, each outside the view beyond the conservative extent bound of its style
- **WHEN** a frame is prepared
- **THEN** none of them SHALL be measured, stored or laid out

### Requirement: The label stage budget follows the labels that can appear

The heap allocation, the measurement calls and the work of the label stage of a frame SHALL be
bounded by the labels that can appear in the view and SHALL NOT grow with the labels that the
loaded objects produce but that cannot intersect the view.

#### Scenario: Off-view labels do not raise the allocation count of the stage

- **GIVEN** a view and the same view with a large number of further loaded objects whose labels cannot intersect the viewport
- **WHEN** both views are prepared repeatedly
- **THEN** the label set, the placement and the draw order SHALL be identical between the two views
- **THEN** the label stage SHALL NOT perform substantially more heap allocations and measurements for the view with the additional off-view labels

#### Scenario: The number of measurements follows the placed labels

- **GIVEN** a view whose label stage measures a known number of labels
- **WHEN** the view is prepared after the change
- **THEN** the number of measurement calls SHALL NOT exceed the number of labels that can intersect the view, plus a constant

### Requirement: Label set, placement, draw order and measurement results are unchanged

For any view, the painter SHALL produce the same label set, the same label placement, the same
draw order and the same measurement results as before this change, including for labels whose
ordering criteria compare equal.

#### Scenario: The label set and placement are unchanged

- **GIVEN** a view whose label set and label placement are known from before the change
- **WHEN** the same view is prepared after the change
- **THEN** the label set SHALL be identical, entry by entry, in text, style, position and order

#### Scenario: The text measurement results are unchanged

- **GIVEN** a label that takes part in the frame
- **WHEN** it is measured after the change
- **THEN** its measured dimensions SHALL be identical to the dimensions measured before the change

#### Scenario: Rendered output is unchanged

- **GIVEN** a view rendered before the change, with a stylesheet that draws labels of every source
- **WHEN** the same view is rendered after the change, with the same stylesheet and parameters
- **THEN** the rendered output SHALL be identical
