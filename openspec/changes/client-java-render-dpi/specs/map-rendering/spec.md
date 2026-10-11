# Spec Delta

## Purpose

Render OSM map data to a JavaFX Canvas using the Cairo renderer via JNI. The rendering pipeline loads tile data from the database, renders with `MapPainterCairo`, and blits the pixel buffer to a JavaFX Canvas.

## MODIFIED Requirements

### Requirement: JNI render method
`OSMScoutClient` SHALL expose a native `render()` method that renders the current map view to an ARGB pixel
array. The request SHALL state the physical DPI of the display the frame is rendered for, and SHALL project
with that value. A request that states no usable DPI SHALL project with the DPI configured on the client,
and the request SHALL NOT change that configured value.

#### Scenario: Render returns pixel data
- **WHEN** `render(width, height, lat, lon, angle, magnification, dpi)` is called
- **THEN** C++ creates `MercatorProjection` with the given center, angle, magnification and DPI
- **THEN** C++ loads tile data from databases via `MapService`
- **THEN** C++ renders via `MapPainterCairo::DrawMap()` to Cairo image surface
- **THEN** C++ converts BGRx to `int[]` ARGB
- **THEN** Java receives non-null `int[]` of length `width * height`

#### Scenario: Render with invalid parameters
- **WHEN** `render()` is called with width=0 or height=0
- **THEN** method returns `null`
- **WHEN** `render()` is called before database is initialized
- **THEN** method returns `null`

#### Scenario: The request's DPI scales the frame
- **GIVEN** two requests that differ only in the DPI they state
- **WHEN** both frames are rendered for the same center, angle and magnification
- **THEN** the frames SHALL be projected with their respective DPI values
- **AND** the labels and icons of the frame rendered at the higher DPI SHALL be smaller in geographic extent

#### Scenario: A request without a usable DPI uses the configured one
- **GIVEN** a client whose configured render DPI is known
- **WHEN** a request states an undefined or non-positive DPI
- **THEN** the frame SHALL be projected with the client's configured DPI
- **AND** the request SHALL NOT change the client's configured DPI
- **AND** the call SHALL NOT be rejected

#### Scenario: One surface's request does not change another surface's frame
- **GIVEN** a client used from two surfaces with different display densities
- **WHEN** each surface states its own DPI in its render request
- **THEN** each frame SHALL be projected with the DPI its own request stated
- **AND** neither request SHALL change the DPI the other frame is rendered with

### Requirement: JNI render method supports optional route, POI markers, and track overlay

The system SHALL expose a native render method that renders the current map view with optional route polyline, favorite/search marker POIs, and imported track polyline. The current-location marker is no longer rendered by the native backend. The request SHALL state the physical DPI of the display the frame is rendered for, under the same rule as the base render method.

#### Scenario: Render with route, markers, and track

- **WHEN** `renderWithRouteAndPois(width, height, lat, lon, angle, mag, dpi, routeLats, routeLons, favoriteLats, favoriteLons, searchSelLat, searchSelLon, trackLats, trackLons)` is called
- **THEN** C++ SHALL create a synthetic `_route` WAY from route coordinates when provided
- **AND** C++ SHALL create synthetic `_favorite`, `_search_selected`, `_route_start`, and `_route_end` NODEs when their coordinates are provided
- **AND** C++ SHALL create a synthetic `_track` WAY from track coordinates when provided
- **AND** C++ SHALL add all synthetic objects to `MapData` before `MapPainterCairo::DrawMap()`
- **AND** C++ SHALL project with the DPI the request stated
- **AND** Java SHALL receive non-null `int[]` of length `width * height`

#### Scenario: Render without track overlay

- **GIVEN** `trackLats` and `trackLons` are null or empty
- **WHEN** the render method is called
- **THEN** no `_track` WAY SHALL be added to `MapData`
- **AND** the map SHALL render without a track overlay

#### Scenario: Render without current location overlay

- **GIVEN** current location rendering is handled by the JavaFX overlay layer
- **WHEN** `renderWithRouteAndPois()` is called without current-location parameters
- **THEN** C++ SHALL not add any synthetic current-location node to `MapData`

#### Scenario: The convenience form states no DPI
- **GIVEN** a caller that uses the render method without a DPI argument
- **WHEN** the call is made
- **THEN** it SHALL forward the request with no DPI stated
- **AND** the frame SHALL be projected with the client's configured DPI
