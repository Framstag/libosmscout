# javascout-track-rendering

## Purpose

Render imported GPX track points as a styled polyline overlay on the JavaScout Cairo map.

## Requirements

### Requirement: MapRenderer accepts track points

The system SHALL allow `MapRenderer` to store a list of imported track points and pass them to the native renderer on every render call.

#### Scenario: Set track points updates renderer state

- **GIVEN** a `MapRenderer` instance with no active track
- **WHEN** `setTrackPoints(TrackPoint[] points)` is called
- **THEN** the renderer SHALL store the latitude/longitude arrays
- **AND** the next render SHALL include the track geometry

#### Scenario: Clear track points removes overlay

- **GIVEN** a `MapRenderer` instance with an active track
- **WHEN** `clearTrack()` is called
- **THEN** the track geometry SHALL be removed
- **AND** the next render SHALL NOT draw the track

### Requirement: Track is preserved across view changes

The system SHALL re-render the track overlay when the map is panned, zoomed, or resized, without requiring re-import.

#### Scenario: Pan keeps track visible

- **GIVEN** a track is rendered on the map
- **WHEN** the user pans the map
- **THEN** the track polyline SHALL remain visible at the correct geographic position

#### Scenario: Zoom keeps track visible

- **GIVEN** a track is rendered on the map
- **WHEN** the user zooms the map
- **THEN** the track polyline SHALL scale with the map

### Requirement: UI imports and displays track from the top-left menu

The system SHALL provide a menu item in a top-left menu that opens a file chooser, imports the selected GPX file, and immediately renders its track on the map.

#### Scenario: Import via menu

- **GIVEN** JavaScout has a loaded database
- **WHEN** the user opens the top-left menu and selects "Import GPX Track…"
- **AND** the user selects a valid GPX file in the file chooser
- **THEN** the track SHALL appear as a polyline on the map

#### Scenario: Import cancels when file chooser closes

- **GIVEN** the file chooser is open
- **WHEN** the user cancels without selecting a file
- **THEN** no track SHALL be imported and the map SHALL remain unchanged

### Requirement: JNI render path draws track polyline

The native render method SHALL convert the track point arrays into an `osmscout::Way` and add it to `MapData::poiWays` before calling the Cairo painter.

#### Scenario: Track with multiple points renders as connected polyline

- **GIVEN** at least two track points
- **WHEN** the native renderer processes the track arrays
- **THEN** it SHALL create a `Way` with one point per track coordinate
- **AND** the Cairo painter SHALL draw a polyline connecting them

#### Scenario: Single track point renders as node marker

- **GIVEN** exactly one track point
- **WHEN** the native renderer processes the track arrays
- **THEN** it SHALL render a visible marker at that coordinate
