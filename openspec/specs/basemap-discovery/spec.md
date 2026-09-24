# Basemap Discovery Specification

## Purpose

Detect whether a world basemap is available on the configured map provider's server, even though it is not listed in the standard map JSON listing.

## Requirements

### Requirement: Probe basemap existence

The system SHALL probe a well-known server location for basemap availability when the user opens
the Map Download dialog or on explicit refresh, and SHALL read the availability manifest the
regeneration pass writes there. The system SHALL NOT depend on a server directory listing.

#### Scenario: Basemap exists on server

- **WHEN** user opens Map Download dialog
- **THEN** system SHALL read the basemap availability manifest at the well-known location
- **THEN** if the manifest names at least one database format version, system SHALL report basemap
  as available
- **THEN** system SHALL report the versions offered and when the newest of them last changed

#### Scenario: Basemap does not exist on server

- **WHEN** system probes the basemap location and receives HTTP 404 or a connection error
- **THEN** system SHALL report basemap as unavailable
- **THEN** system SHALL NOT show an error to the user (basemap is optional)

#### Scenario: Manifest is unreadable or contradicts itself

- **GIVEN** a manifest that cannot be parsed, or that names a version whose data is absent
- **WHEN** the system probes the basemap location
- **THEN** system SHALL report basemap as unavailable and SHALL NOT offer it for download
- **THEN** system SHALL NOT show an error to the user

### Requirement: Report basemap version

The system SHALL determine from the availability manifest whether a basemap it can read is
available, and SHALL determine whether an installed basemap is older than the newest version the
server offers.

#### Scenario: Basemap update available

- **WHEN** the installed basemap's recorded change time is older than the newest server version's
  change time, or the installed database format version differs from the version the server newest
  offers
- **THEN** system SHALL indicate an update is available in the UI

#### Scenario: No basemap installed

- **WHEN** no basemap directory exists locally
- **THEN** system SHALL report basemap as available for initial download

#### Scenario: Installed basemap is current

- **GIVEN** an installed basemap whose recorded version and change time equal the newest server
  version
- **WHEN** the system probes the basemap location
- **THEN** system SHALL report no update as available

### Requirement: Choose a readable version

The system SHALL offer the newest basemap version on the server that the system's library can read,
and SHALL NOT offer a basemap whose database format version the library cannot read. When the
server offers only versions the system cannot read, the system SHALL report the basemap as
unavailable rather than as an error.

#### Scenario: Newest readable version is chosen

- **GIVEN** a server offering several basemap versions and a library that supports some of them
- **WHEN** the system determines what to offer
- **THEN** it SHALL offer the newest version the library supports
- **AND** it SHALL NOT offer a version newer than the library supports

#### Scenario: No readable version on the server

- **GIVEN** a server offering only basemap versions newer than the library supports
- **WHEN** the system determines what to offer
- **THEN** it SHALL report the basemap as unavailable to this system
- **THEN** it SHALL NOT report a download error
