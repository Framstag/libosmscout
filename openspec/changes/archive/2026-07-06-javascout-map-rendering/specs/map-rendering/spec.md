## ADDED Requirements
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
