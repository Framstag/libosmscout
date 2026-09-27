# Basemap Loading Specification

## Purpose

Load the world basemap as an overlay on application startup so that borders, country names, and coastlines render underneath regional maps, providing context when zoomed out or when no regional map is loaded.

## Requirements

### Requirement: Pass basemap directory to C++ layer on startup

The system SHALL pass the basemap directory path to `OSMScoutClientBuilder.withBasemapLookupDirectory()` during client initialization if a basemap is installed.

#### Scenario: Basemap installed at startup
- **WHEN** JavaScout starts
- **WHEN** basemap directory exists at `{mapsDir}/basemap/`
- **THEN** system calls `builder.withBasemapLookupDirectory("{mapsDir}/basemap/")`
- **THEN** C++ DBThread loads basemap as an overlay database

#### Scenario: No basemap installed at startup
- **WHEN** JavaScout starts
- **WHEN** no basemap directory exists
- **THEN** system SHALL NOT call `withBasemapLookupDirectory()`
- **THEN** system SHALL start normally without basemap overlay

### Requirement: Reload basemap after download

The system SHALL reload the basemap when a new basemap is downloaded or updated while the application is running.

#### Scenario: Basemap downloaded while app is running
- **WHEN** user downloads or updates basemap
- **THEN** system SHALL trigger a database list change to reload the basemap
- **THEN** system SHALL re-render the current view with basemap overlay active

### Requirement: Basemap renders underneath regional maps

The basemap SHALL render as a background layer, with regional maps drawn on top. When no regional map covers the current view, the basemap SHALL still be visible.

#### Scenario: Viewing area with no regional map
- **WHEN** user pans to a region with no installed regional map
- **THEN** basemap borders, country names, and coastlines SHALL still be visible
- **THEN** system SHALL NOT show a blank/empty map

#### Scenario: Viewing area with regional map
- **WHEN** user views an area covered by an installed regional map
- **THEN** regional map data SHALL render on top of basemap data
- **THEN** basemap SHALL provide context at low zoom levels where regional map detail is sparse

### Requirement: Set basemap directory on a running client

The system SHALL let an application set the basemap directory while the application runs, replacing the directory configured when the client was created. Setting an empty directory SHALL unload the basemap. The request SHALL be accepted without blocking the calling thread, and the basemap SHALL be reloaded so the change takes effect without restarting the application; whether a basemap is available after the request SHALL be observable from the client.

#### Scenario: Basemap installed while the app runs

- **GIVEN** a running client with no basemap loaded
- **WHEN** the application sets the basemap directory to a directory holding an installed basemap
- **THEN** the basemap SHALL load and SHALL be rendered as an overlay without restarting the application

#### Scenario: Basemap directory cleared

- **GIVEN** a running client with a basemap loaded
- **WHEN** the application sets the basemap directory to an empty value
- **THEN** the basemap SHALL be unloaded
- **AND** the remaining maps SHALL keep rendering

#### Scenario: Basemap directory holds no usable basemap

- **GIVEN** a running client
- **WHEN** the application sets the basemap directory to a directory with no openable basemap database
- **THEN** no basemap layer SHALL be drawn
- **AND** the remaining maps SHALL keep rendering

#### Scenario: Setting the basemap directory does not block the caller

- **GIVEN** a running client rendering a map
- **WHEN** the application sets the basemap directory
- **THEN** the call SHALL return without waiting for the basemap to finish loading
- **AND** the map SHALL keep rendering during the reload

#### Scenario: Directory supplied at client creation still honoured

- **GIVEN** a client created with a basemap directory
- **WHEN** the client is used without setting the directory again
- **THEN** the basemap SHALL load from the directory supplied at creation
