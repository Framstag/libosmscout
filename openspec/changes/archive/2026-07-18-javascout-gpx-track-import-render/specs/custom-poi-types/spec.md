# custom-poi-types

## Purpose

Allow JavaScout (and other Java clients) to register synthetic POI types at runtime so that overlay markers can be rendered through the standard map style pipeline without modifying imported `.osmscout` databases.

## ADDED Requirements

### Requirement: _track synthetic POI type is registered in JavaScout

The system SHALL register a `_track` synthetic POI type in JavaScout so imported GPX tracks can be rendered through the standard style pipeline.

#### Scenario: MainController registers track type

- **WHEN** `MainController` initializes the `OSMScoutClient`
- **THEN** it SHALL call `withCustomPoiType("_track")` in addition to the existing custom POI types

#### Scenario: Track type is available for styling

- **GIVEN** `_track` was registered during client build
- **WHEN** a map database is opened
- **THEN** the database's `TypeConfig` SHALL contain the type `_track`
