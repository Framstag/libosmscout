## ADDED Requirements

### Requirement: Route rendered through Cairo pipeline

JavaScout SHALL render the computed route through the existing Cairo map rendering pipeline, not as a separate JavaFX overlay. The route waypoints SHALL be passed back to the JNI `render()` method, converted to an `osmscout::Way` with type `_route`, and added to `MapData::poiWays` before `MapPainterCairo::DrawMap()`. Start marker SHALL use type `_route_start`, end marker SHALL use type `_route_end`.

#### Scenario: Route renders after calculation
- **WHEN** a route is successfully calculated
- **THEN** the route waypoints are passed to the next `render()` call
- **AND** the Cairo painter draws a colored polyline from start to destination
- **AND** a start marker and end marker are rendered at the route endpoints

#### Scenario: Route updates on recalculation
- **WHEN** a new route is calculated (different start or destination)
- **THEN** the previous route waypoints are replaced with the new ones
- **AND** the next `render()` call draws the updated route

#### Scenario: Route clears on input change
- **WHEN** start or destination field is cleared
- **THEN** route waypoints are cleared and no route is rendered on next `render()` call

#### Scenario: Route re-renders on map view change
- **WHEN** the user pans or zooms the map
- **THEN** the route is re-rendered at the new view position/zoom via the normal render pipeline
