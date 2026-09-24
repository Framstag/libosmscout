# Spec Delta

## ADDED Requirements

### Requirement: Client selects the stylesheet the basemap renders with

The client SHALL let an application name the stylesheet the basemap renders with, resolved against the same stylesheets directory as the map styles, so a basemap with its own type definitions is presented by its own stylesheet instead of the active map style. A name given with or without the stylesheet file extension SHALL be accepted. Naming a basemap stylesheet SHALL NOT change the active map style.

When no basemap stylesheet is named, or the name is not a plain stylesheet name (empty, containing a path separator, or `.` / `..`), the basemap SHALL render with the active map style. A named basemap stylesheet that is resolved but cannot be loaded SHALL never leave the basemap without a usable style configuration: the basemap layer SHALL be dropped for that render, map rendering SHALL continue, and the failure SHALL be reported like any other failed stylesheet load.

#### Scenario: Basemap stylesheet selected

- **GIVEN** a client with the stylesheets directory configured and map style "standard" active
- **WHEN** the application creates the client naming "basemap-render" as the basemap stylesheet
- **THEN** the basemap SHALL render with "basemap-render"
- **AND** the active map style SHALL remain "standard"

#### Scenario: Basemap stylesheet named with its file extension

- **GIVEN** a client with the stylesheets directory configured
- **WHEN** the application names the basemap stylesheet as "basemap-render.oss"
- **THEN** the basemap SHALL render with the stylesheet "basemap-render"

#### Scenario: No basemap stylesheet named

- **GIVEN** a client created without a basemap stylesheet
- **WHEN** the basemap is loaded
- **THEN** the basemap SHALL render with the active map style

#### Scenario: Basemap stylesheet name that is not a plain name

- **GIVEN** an application names a basemap stylesheet that is empty, contains a path separator, or is `.` or `..`
- **WHEN** the client is created
- **THEN** the client SHALL treat the selection as not named
- **AND** the basemap SHALL render with the active map style

#### Scenario: Basemap stylesheet name with no matching stylesheet

- **GIVEN** a client whose stylesheets directory contains no "basemap-render" stylesheet
- **WHEN** the application names "basemap-render" as the basemap stylesheet
- **THEN** the client SHALL treat the selection as not named
- **AND** the basemap SHALL render with the active map style
- **AND** the client SHALL be created without failing

#### Scenario: Named basemap stylesheet cannot be loaded

- **GIVEN** a client created naming a basemap stylesheet whose file cannot be parsed
- **WHEN** the basemap is loaded while the map stylesheet loads successfully
- **THEN** map rendering SHALL continue with the map stylesheet
- **AND** the basemap layer SHALL NOT be drawn with a failed configuration
