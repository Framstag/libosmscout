# Basemap Discovery Specification

## Purpose

Detect whether a world basemap is available on the configured map provider's server, even though it is not listed in the standard map JSON listing.

## Requirements

### Requirement: Probe basemap existence

The system SHALL probe a well-known server path for basemap availability when the user opens the Map Download dialog or on explicit refresh.

#### Scenario: Basemap exists on server
- **WHEN** user opens Map Download dialog
- **THEN** system probes `{provider.uri}/basemap/` for basemap archives
- **THEN** if a tar.gz archive exists, system SHALL report basemap as available
- **THEN** system SHALL report the latest archive name and date

#### Scenario: Basemap does not exist on server
- **WHEN** system probes basemap path and receives HTTP 404 or connection error
- **THEN** system SHALL report basemap as unavailable
- **THEN** system SHALL NOT show an error to the user (basemap is optional)

### Requirement: Report basemap version

The system SHALL read the basemap version from the server (e.g., from a metadata file or directory listing) to determine if an update is available.

#### Scenario: Basemap update available
- **WHEN** installed basemap version is older than server version
- **THEN** system SHALL indicate an update is available in the UI

#### Scenario: No basemap installed
- **WHEN** no basemap directory exists locally
- **THEN** system SHALL report basemap as available for initial download
