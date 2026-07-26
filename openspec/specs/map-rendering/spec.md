## Purpose

Render OSM map data to a JavaFX Canvas using the Cairo renderer via JNI. The rendering pipeline loads tile data from the database, renders with `MapPainterCairo`, and blits the pixel buffer to a JavaFX Canvas.

## Requirements

### Requirement: Render map to JavaFX Canvas
The system SHALL render OSM map data to a JavaFX Canvas using the Cairo renderer via JNI.

#### Scenario: Initial render after database load
- **WHEN** database finishes loading
- **THEN** system calls `OSMScoutClient.render(w, h, lat, lon, mag)` via JNI
- **THEN** system receives `int[]` ARGB pixel data
- **THEN** system writes pixels to Canvas via `PixelWriter.setPixels()`
- **THEN** map is visible in the center panel

#### Scenario: Re-render on view change
- **WHEN** user pans or zooms the map
- **THEN** system calls `render()` with new lat/lon/mag after 200ms debounce
- **THEN** Canvas updates with new rendered frame

#### Scenario: Render with no database
- **WHEN** no database is loaded
- **THEN** Canvas shows empty/background color
- **THEN** no JNI render call is made

### Requirement: Renderer is initialised with a valid icon directory
The system SHALL ensure the renderer is initialised with a valid icon directory so POI icons can be drawn.

#### Scenario: Renderer has icon paths configured
- **WHEN** the native client is built with a non-empty icon directory
- **THEN** `MapParameter::SetIconPaths()` SHALL be called before rendering
- **THEN** `MapPainterCairo` SHALL use the configured directory to locate icons

### Requirement: Renderer receives the configured icon directory
The system SHALL pass the configured icon directory to the Cairo renderer via `MapParameter::SetIconPaths()`.

#### Scenario: Icon directory reaches the renderer
- **WHEN** the native client is built with a non-empty icon directory
- **THEN** `OSMScoutClient.cpp` SHALL call `params.SetIconPaths({iconDir})` before `MapPainterCairo::DrawMap()`
- **THEN** the renderer SHALL search for icons in that directory

#### Scenario: POI icons rendered when icon directory is configured
- **WHEN** the native client is built with a non-empty icon directory pointing to PNG icons
- **THEN** JavaScout SHALL append a trailing path separator to the directory before passing it to the builder
- **THEN** the renderer SHALL load `.png` icons from that directory
- **THEN** POI icons SHALL appear on the rendered map

#### Scenario: No icons rendered when icon directory is empty
- **WHEN** the native client is built without an icon directory
- **THEN** the renderer SHALL not attempt to load icons
- **THEN** the map SHALL render without POI icons

#### Scenario: SVG icons are not loaded by the Cairo renderer
- **WHEN** the configured icon directory contains only SVG files
- **THEN** the Cairo renderer SHALL fail to load those icons
- **THEN** an error message SHALL be logged for each missing PNG icon

### Requirement: JNI render method
`OSMScoutClient` SHALL expose a native `render()` method that renders the current map view to an ARGB pixel array.

#### Scenario: Render returns pixel data
- **WHEN** `render(width, height, lat, lon, magnification)` is called
- **THEN** C++ creates `MercatorProjection` with given parameters
- **THEN** C++ loads tile data from databases via `MapService`
- **THEN** C++ renders via `MapPainterCairo::DrawMap()` to Cairo image surface
- **THEN** C++ converts BGRx to `int[]` ARGB
- **THEN** Java receives non-null `int[]` of length `width * height`

#### Scenario: Render with invalid parameters
- **WHEN** `render()` is called with width=0 or height=0
- **THEN** method returns `null`
- **WHEN** `render()` is called before database is initialized
- **THEN** method returns `null`

### Requirement: JNI render method supports optional route, POI markers, and track overlay

The system SHALL expose a native render method that renders the current map view with optional route polyline, favorite/search marker POIs, and imported track polyline. The current-location marker is no longer rendered by the native backend.

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

#### Scenario: Render without current location overlay

- **GIVEN** current location rendering is handled by the JavaFX overlay layer
- **WHEN** `renderWithRouteAndPois()` is called without current-location parameters
- **THEN** C++ SHALL not add any synthetic current-location node to `MapData`

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

### Requirement: JNI exposes map projection helper
The system SHALL expose a native helper that projects a geographic coordinate to viewport pixel coordinates for a given map view, so Java-side overlays can be positioned without reimplementing the projection.

#### Scenario: Project geographic coordinate to pixel
- **WHEN** `projectToPixel(width, height, centerLat, centerLon, magnification, dpi, lat, lon)` is called
- **THEN** C++ SHALL create the same `MercatorProjection` used for rendering
- **AND** C++ SHALL return `double[]{x, y}` in viewport pixel coordinates, or null if invalid/outside
