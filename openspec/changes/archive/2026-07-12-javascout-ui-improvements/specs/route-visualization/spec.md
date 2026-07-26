## MODIFIED Requirements

### Requirement: Route rendered through Cairo pipeline

JavaScout SHALL render the computed route through the existing Cairo map rendering pipeline, not as a separate JavaFX overlay. The route waypoints SHALL be passed back to the JNI `renderWithRoute()` method, converted to an `osmscout::Way` with type `_route`, and added to `MapData::poiWays` before `MapPainterCairo::DrawMap()`. Start marker SHALL use type `_route_start`, end marker SHALL use type `_route_end`. The route SHALL be preserved across all map renders triggered by pan, zoom, and resize.

#### Scenario: Route renders after calculation
- **WHEN** a route is successfully calculated
- **THEN** the route waypoints are passed to the next `renderWithRoute()` call
- **AND** the Cairo painter draws a colored polyline from start to destination
- **AND** a start marker and end marker are rendered at the route endpoints

#### Scenario: Route updates on recalculation
- **WHEN** a new route is calculated (different start or destination)
- **THEN** the previous route waypoints are replaced with the new ones
- **AND** the next `renderWithRoute()` call draws the updated route

#### Scenario: Route clears on input change
- **WHEN** start or destination field is cleared
- **THEN** route waypoints are cleared and no route is rendered on next `renderWithRoute()` call

#### Scenario: Route re-renders on map view change
- **WHEN** the user pans or zooms the map
- **THEN** the route is re-rendered at the new view position/zoom via the normal render pipeline

#### Scenario: Route re-renders on window resize
- **WHEN** the user resizes the window
- **THEN** the map SHALL redraw to the new size
- **AND** the route waypoints SHALL be passed to the render call
- **AND** the route SHALL remain visible

## ADDED Requirements

### Requirement: Route instructions are presented as web-like cards
After a route is calculated, JavaScout SHALL display turn-by-turn instructions in a scrollable, web-like card list.

#### Scenario: Route instructions appear as cards
- **WHEN** a route is successfully calculated
- **THEN** the route panel SHALL show a scrollable list of instruction cards
- **AND** each card SHALL display a primary instruction (turn direction and road name) prominently
- **AND** each card SHALL display distance and time as secondary, smaller metadata

#### Scenario: Cards are styled and interactive
- **WHEN** the user hovers over an instruction card
- **THEN** the card SHALL highlight to indicate interactivity
- **AND** cards SHALL have a consistent border and background matching the dialog styling

### Requirement: Route instruction parser supports optical navigation hints
JavaScout SHALL parse route description strings into a structured {@link RouteInstruction} object that can be extended with optical navigation hints.

#### Scenario: Parser extracts turn direction, road type, road name, and metadata
- **GIVEN** a route description line such as "Left onto highway_primary Evinger Straße (B 54)  [0.1 km, 0 min]"
- **WHEN** the parser processes the line
- **THEN** it SHALL produce a {@link RouteInstruction} with turn icon, primary text, secondary distance/time, road type, and road name
- **AND** the structured object SHALL be used to render the instruction card

#### Scenario: Parser handles start and destination lines
- **GIVEN** a route description line starting with "Start:" or "Destination:"
- **WHEN** the parser processes the line
- **THEN** it SHALL assign appropriate start/destination icons and text

#### Scenario: Route instruction cards support keyboard navigation
- **GIVEN** the route panel is expanded and showing instruction cards
- **WHEN** the user presses the Up or Down arrow keys
- **THEN** focus SHALL move between instruction cards
- **AND** the focused card SHALL be visually highlighted

#### Scenario: Route panel closes with Escape key
- **GIVEN** the route panel is expanded
- **WHEN** the user presses the Escape key
- **THEN** the route panel SHALL collapse
