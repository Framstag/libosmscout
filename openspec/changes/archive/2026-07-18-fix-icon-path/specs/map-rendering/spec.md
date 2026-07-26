## MODIFIED Requirements

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
