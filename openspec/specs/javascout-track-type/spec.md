# javascout-track-type

## Purpose

Define a synthetic `_track` map type that the JavaScout renderer can register at runtime and style through the standard stylesheet pipeline.

## Requirements

### Requirement: `_track` type is registered as synthetic POI type

The system SHALL register `_track` as a custom POI type via `OSMScoutClientBuilder.withCustomPoiType("_track")` during JavaScout client initialization.

#### Scenario: JavaScout client initialization registers track type

- **WHEN** `MainController` builds the `OSMScoutClient`
- **THEN** it SHALL call `withCustomPoiType("_track")`
- **AND** the resulting type config SHALL contain `_track`

### Requirement: `_track` type can be styled in route.oss

The system SHALL provide a default WAY style for `_track` in `stylesheets/include/route.oss`.

#### Scenario: Track has visible default style

- **GIVEN** `_track` is registered and a track way is added to `MapData::poiWays`
- **WHEN** the map is rendered at detail zoom or higher
- **THEN** the track SHALL be visible as a colored polyline

#### Scenario: Track is hidden at low zoom

- **GIVEN** a track is present in the viewport
- **WHEN** the map is rendered below detail zoom
- **THEN** the track SHALL NOT be visible to avoid clutter

### Requirement: `_track` style has appropriate display width

The system SHALL render the track polyline with a display width suitable for a recorded or planned path.

#### Scenario: Track width is distinguishable from route

- **GIVEN** both a route (`_route`) and a track (`_track`) are rendered
- **WHEN** the map is rendered
- **THEN** the track SHALL be visually distinguishable from the route by width or color
