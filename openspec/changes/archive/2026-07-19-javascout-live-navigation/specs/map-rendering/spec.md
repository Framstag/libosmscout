# map-rendering

## ADDED Requirements

### Requirement: Renderer can draw current-location marker during navigation
The system SHALL render a current-location marker on the map when a navigation position estimate is active.

#### Scenario: Render with current location
- **WHEN** a navigation position is available and the map is rendered
- **THEN** JavaScout paints the marker on top of the Canvas after the base map image is blitted
- **AND** the marker is always drawn last, so it appears above roads, POIs, route and track overlays

### Requirement: Renderer can draw heading indicator during navigation
The system SHALL render a heading/accuracy indicator for the current location when heading and accuracy information are available.

#### Scenario: Render heading indicator
- **WHEN** a bearing and horizontal accuracy are available
- **THEN** JavaScout draws a direction arrow aligned with the bearing
- **AND** marker tint reflects GPS quality (blue=unknown, green=good, yellow=moderate, red=poor)

## MODIFIED Requirements

### Requirement: JNI render method supports optional route, POI markers, and track overlay
The system SHALL expose a native render method that renders the current map view with optional route polyline, favorite/search marker POIs, and imported track polyline. The current-location marker is no longer rendered by the native backend.

#### Scenario: Render with route, markers, and track
- **WHEN** `renderWithRouteAndPois(width, height, lat, lon, mag, routeLats, routeLons, favoriteLats, favoriteLons, searchSelLat, searchSelLon, trackLats, trackLons)` is called
- **THEN** C++ SHALL render the map with `MapPainterCairo::DrawMap()`
- **AND** Java SHALL receive non-null `int[]` of length `width * height`

### Requirement: JNI exposes map projection helper
The system SHALL expose a native helper that projects a geographic coordinate to viewport pixel coordinates for a given map view, so Java-side overlays can be positioned without reimplementing the projection.

#### Scenario: Project geographic coordinate to pixel
- **WHEN** `projectToPixel(width, height, centerLat, centerLon, magnification, dpi, lat, lon)` is called
- **THEN** C++ SHALL create the same `MercatorProjection` used for rendering
- **AND** C++ SHALL return `double[]{x, y}` in viewport pixel coordinates, or null if invalid/outside
