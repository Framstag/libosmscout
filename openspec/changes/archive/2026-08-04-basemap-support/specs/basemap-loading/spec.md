## Purpose

Load the world basemap as an overlay on application startup so that borders, country names, and coastlines render underneath regional maps, providing context when zoomed out or when no regional map is loaded.

## ADDED Requirements

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
