# route-visualization

Route polyline rendering on map via Cairo pipeline.

## Purpose

Render computed route as an overlay on the map using the existing Cairo rendering pipeline. Route waypoints are passed to the JNI `renderWithRoute()` method, converted to an `osmscout::Way` with type `_route`, and added to `MapData::poiWays` before `MapPainterCairo::DrawMap()`.

## Requirements

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

### Requirement: Route type definitions in stylesheet

The `_route` type is registered at runtime in `TypeConfig::Initialize`; `_route_start` and `_route_end` are registered at runtime by the clients (JNI `withCustomPoiType`, Qt `AddCustomPoiType`). Rendering styles for all three SHALL be defined in `stylesheets/include/route.oss`.

The route paint SHALL follow the presentation the style sheet is loaded with, and SHALL define a daylight and a dark variant of the fill colour and of the casing colour in that module. The daylight fill SHALL differ from the road colours of the style sheets it is drawn over, and the daylight casing SHALL stay visible on the lightest road fill the style sheets use. The daylight pair SHALL keep a black way label that is drawn directly on the route centre readable: the casing SHALL be opaque and wider than the fill, so the composited centre the label sits on does not take the colour of the road underneath.

#### Scenario: Route polyline has visible style

- **GIVEN** a style sheet loaded with the daylight presentation
- **WHEN** a route is rendered
- **THEN** the `_route` WAY styles from `route.oss` are applied as a cased line: a translucent violet fill over an opaque, wider violet casing
- **AND** the cased line is visible on top of red primary roads
- **AND** the composited centre of the cased line keeps its contrast against a black way label drawn on it

#### Scenario: Route polyline is distinguishable in the dark presentation

- **GIVEN** a style sheet loaded with the dark presentation
- **WHEN** a route is rendered
- **THEN** the `_route` WAY styles are applied as a cased line with the dark variant of the fill and the casing colours
- **AND** the casing of the dark variant SHALL be a light colour, so the route stays visible over the darkened map

#### Scenario: Route start and end markers render

- **WHEN** a route is rendered
- **THEN** the `_route_start` and `_route_end` NODE.ICON styles from `route.oss` are applied at the route endpoints

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

### Requirement: Style sheets share the route paint

Every style sheet that can draw an active route SHALL take the route's paint from `stylesheets/include/route.oss` and SHALL NOT define a route line style or a route colour of its own, so that the route looks the same whichever of those style sheets is active and a colour change has one place to be made. A style sheet that includes the module SHALL resolve the same casing and fill styles as the other style sheets that include it.

#### Scenario: Style sheets that draw a route resolve the shared paint

- **GIVEN** the style sheets that can draw an active route
- **WHEN** the route line styles of each are resolved with the same presentation
- **THEN** each of them SHALL resolve the shared module's casing style and fill style
- **AND** the colours SHALL be the same in every one of them

#### Scenario: The cycle style sheet no longer paints its own route

- **GIVEN** the cycle style sheet
- **WHEN** its route line styles are resolved
- **THEN** they SHALL be the casing and the fill of the shared module
- **AND** the style sheet SHALL NOT carry a route colour of its own

#### Scenario: Adding the shared module brings the shared overlay markers

- **GIVEN** a style sheet that includes the shared route module
- **WHEN** the styles of the route start marker, the route end marker, the imported track and the favourite and search-selection markers are resolved
- **THEN** they SHALL be defined by that module, so a style sheet cannot include the route paint without the markers that belong to the same overlay presentation
