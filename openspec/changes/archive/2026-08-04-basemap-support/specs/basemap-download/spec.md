## Purpose

Download the world basemap from the configured map provider and store it in a well-known local directory for use as an overlay map.

## ADDED Requirements

### Requirement: Download basemap archive

The system SHALL download the basemap tar.gz archive from the provider and extract it into a dedicated subdirectory (`{mapsDir}/basemap/`).

#### Scenario: Successful basemap download
- **WHEN** user initiates basemap download
- **THEN** system downloads the selected tar.gz archive from `{provider.uri}/basemap/{archive}`
- **THEN** system extracts the archive into `{mapsDir}/basemap/`
- **THEN** system reports download and extraction progress
- **THEN** system registers the basemap directory with the map manager

#### Scenario: Basemap download failure
- **WHEN** basemap download fails (network error, partial archive)
- **THEN** system SHALL clean up partial files
- **THEN** system SHALL report the error to the user
- **THEN** system SHALL NOT corrupt any existing basemap installation

### Requirement: Select basemap variant

The system SHALL allow the user to choose between available basemap variants (full vs minimal) when multiple archives exist on the server.

#### Scenario: Multiple variants available
- **WHEN** server has both full and minimal basemap archives
- **THEN** system SHALL present both options to the user
- **THEN** system SHALL show size and date for each variant

### Requirement: Update basemap

The system SHALL support re-downloading the basemap when a newer version is available.

#### Scenario: Update existing basemap
- **WHEN** user triggers basemap update
- **THEN** system downloads new archive to a temporary file
- **THEN** system extracts to a temporary directory
- **THEN** on success, system replaces old basemap directory atomically
- **THEN** system reloads the basemap in the rendering engine

### Requirement: Cancel basemap download

The system SHALL support cancelling an in-progress basemap download.

#### Scenario: Cancel during download
- **WHEN** user cancels basemap download
- **THEN** system stops the HTTP transfer
- **THEN** system removes any partial files
- **THEN** system SHALL NOT affect any previously installed basemap
