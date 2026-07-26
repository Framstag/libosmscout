# map-rendering

## Purpose

Render OSM map data to a JavaFX Canvas using the Cairo renderer via JNI. The rendering pipeline loads tile data from the database, renders with `MapPainterCairo`, and blits the pixel buffer to a JavaFX Canvas.

## MODIFIED Requirements

### Requirement: JNI render method supports optional route, POI markers, and track overlay

The system SHALL expose a native render method that renders the current map view with optional route polyline, favorite/search marker POIs, and imported track polyline.

#### Scenario: Render with route, markers, and track

- **WHEN** `renderWithRouteAndPois(width, height, lat, lon, mag, routeLats, routeLons, favoriteLats, favoriteLons, searchSelLat, searchSelLon, trackLats, trackLons)` is called
- **THEN** C++ SHALL create a synthetic `_route` WAY from route coordinates when provided
- **AND** C++ SHALL create synthetic `_favorite`, `_search_selected`, `_route_start`, and `_route_end` NODEs when their coordinates are provided
- **AND** C++ SHALL create a synthetic `_track` WAY from track coordinates when provided
- **AND** C++ SHALL add all synthetic objects to `MapData` before `MapPainterCairo::DrawMap()`
- **AND** Java SHALL receive non-null `int[]` of length `width * height`

#### Scenario: Render without track overlay

- **GIVEN** `trackLats` and `trackLons` are null or empty
- **WHEN** the render method is called
- **THEN** no `_track` WAY SHALL be added to `MapData`
- **AND** the map SHALL render without a track overlay
