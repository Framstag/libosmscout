# Spec Delta

## Purpose

Describe the POI search API exposed by `OSMScoutClient` for finding points of interest of given categories within a radius around a coordinate.

## ADDED Requirements

### Requirement: The POI search entry point carries the request's generation

`searchPOIsByTypes` SHALL carry the request's generation as its last parameter, so a POI search can be
superseded or cancelled without touching another surface's search. The category entry point SHALL begin the
poi slot's request, run the search with its generation and end it, so a caller that manages nothing still
supersedes its predecessor. An aborted POI search SHALL return an empty array.

#### Scenario: The generation reaches the native search

- **GIVEN** a caller that began a poi request
- **WHEN** it calls `searchPOIsByTypes` with that generation
- **THEN** the native search SHALL ask that generation at its abort checks

#### Scenario: The category entry point manages its own request

- **GIVEN** a caller that uses the category entry point
- **WHEN** it searches twice in a row
- **THEN** each call SHALL begin and end the poi slot's request itself
- **AND** the second call SHALL not be blocked by the first

#### Scenario: A cancelled POI search returns an empty array

- **GIVEN** a POI search whose generation is cancelled
- **WHEN** the entry point returns
- **THEN** it SHALL return an empty array
