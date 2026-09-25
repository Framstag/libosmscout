## Purpose

Defines the contract for the map painter's early rejection of loaded point objects - nodes
and POI nodes - whose visual extent cannot reach the view. It exists so that the cost of
preparing point objects follows the objects a frame can actually show rather than the objects
the viewport happens to load, while the prepared label elements, the placement and the
rendered output stay exactly as they are.

## ADDED Requirements

### Requirement: A point object that cannot be visible is rejected before its preparation

For every loaded point object, the painter SHALL NOT resolve any of the object's styles, SHALL
NOT build any of its label elements and SHALL NOT register any label element for it when the
object cannot contribute a pixel to the current view.

Where the stylesheet defines no label style for the object's type at the level of the frame, only
the object's icon and symbol can reach the view and their extent is bounded by the stylesheet, so
the decision SHALL be taken before any style of the object is resolved. Where the type has label
styles at that level, the decision SHALL be taken before the object's label elements are built and
registered, because the extent of a label is set by its text and only the resolved styles produce
that text.

The decision SHALL be conservative: it SHALL NOT reject an object whose icon, symbol or label
the existing viewport decision of the label stage would have drawn, for the current stylesheet,
zoom level and map parameters.

#### Scenario: A point object of a type without a label style contributes nothing

- **GIVEN** a loaded point object whose type resolves no label style at the level of the frame and whose position lies outside the current viewport by more than the largest icon and symbol extent the current stylesheet can produce
- **WHEN** a frame is prepared
- **THEN** no style SHALL be resolved for that object
- **THEN** no label element SHALL be registered for that object
- **THEN** the preparation of the frame SHALL NOT perform a heap allocation per such object

#### Scenario: A point object whose labels cannot reach the view contributes nothing

- **GIVEN** a loaded point object whose position and whose label and icon rectangles lie outside the current viewport beyond the conservative extent bound
- **WHEN** a frame is prepared
- **THEN** no label element SHALL be registered for that object
- **THEN** the label stage SHALL NOT measure a label for that object
- **THEN** the frame's label set SHALL be identical to the label set of the same view without that object

#### Scenario: A point object inside the view is prepared as before

- **GIVEN** a point object that is styled in the current stylesheet and lies inside the current viewport
- **WHEN** a frame is prepared
- **THEN** the same icon, symbol and text elements SHALL be registered for it, with the same text, the same positions, the same priorities and the same draw order as before this change

#### Scenario: A point object near the viewport edge keeps its elements

- **GIVEN** a point object whose position lies outside the viewport but whose label rectangle or icon rectangle intersects the viewport
- **WHEN** a frame is prepared and rendered
- **THEN** the object's elements SHALL still be registered and drawn, so that the rendered output matches the output before this change

### Requirement: Point object preparation work follows the visible objects, not the loaded ones

The work and the heap allocation of point object preparation SHALL follow the point objects that
can be visible in the view: an object whose type has no label style at the level of the frame and
whose icon and symbol cannot reach the view SHALL add neither a style resolution nor a heap
allocation, and an object whose label elements cannot reach the view SHALL add no label element
to the frame.

#### Scenario: Additional loaded point objects of a type without a label style do not add allocations

- **GIVEN** a view and the same view with a large number of further loaded point objects of a type that resolves no label style at the level of the frame, whose positions lie outside the viewport beyond the conservative extent bound
- **WHEN** both views are prepared repeatedly
- **THEN** the registered label elements and their placement SHALL be identical between the two views
- **THEN** the preparation of the frame SHALL perform no heap allocation per such object

#### Scenario: The registered element set does not follow the loaded object count

- **GIVEN** two views that register a comparable number of label elements but load very different numbers of point objects outside the view
- **WHEN** both views are prepared
- **THEN** the registered label elements SHALL be identical, and the label elements SHALL not grow with the loaded objects

### Requirement: Prepared label elements, placement and rendered output are unchanged

For any view, the painter SHALL register the same label elements for the same point objects,
with the same text, icons, symbols, positions, priorities and draw order, and SHALL produce
the same rendered output as before this change, including for equal-comparing elements.

#### Scenario: The registered element set is unchanged

- **GIVEN** a view whose registered label elements are known from before the change, including objects at the viewport edge whose labels extend into the view
- **WHEN** the same view is prepared after the change
- **THEN** the registered element set SHALL be identical, element by element, in text, type, position and order

#### Scenario: Rendered output is unchanged

- **GIVEN** a view rendered before the change, with a stylesheet that draws icons, symbols and labels for point objects
- **WHEN** the same view is rendered after the change, with the same stylesheet and parameters
- **THEN** the rendered output SHALL be identical
