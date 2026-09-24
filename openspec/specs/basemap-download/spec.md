# Basemap Download Specification

## Purpose

Download the world basemap from the configured map provider and store it in a well-known local directory for use as an overlay map.

## Requirements

### Requirement: Download basemap archive

The system SHALL download the basemap as a database directory: first the metadata the server
publishes for the chosen version, then each data file that metadata names, into a dedicated
subdirectory (`{mapsDir}/basemap/`). The system SHALL keep the downloaded metadata in that
subdirectory, so that the version and change time of the installed basemap are known locally
without contacting the server.

#### Scenario: Successful basemap download

- **WHEN** user initiates basemap download
- **THEN** system downloads the metadata of the chosen version from
  `{provider.uri}/basemap/v{version}/`
- **THEN** system downloads every data file the metadata names, from the same location
- **THEN** system keeps the metadata in `{mapsDir}/basemap/`
- **THEN** system reports download progress
- **THEN** system registers the basemap directory with the map manager

#### Scenario: Basemap download failure

- **WHEN** basemap download fails (network error, missing file, connection lost)
- **THEN** system SHALL clean up partial files
- **THEN** system SHALL report the error to the user
- **THEN** system SHALL NOT corrupt any existing basemap installation

### Requirement: Update basemap

The system SHALL support re-downloading the basemap when a newer version is available, using the
locally kept metadata to decide that a newer version exists.

#### Scenario: Update existing basemap

- **WHEN** user triggers basemap update
- **THEN** system downloads the metadata and data files of the newer version into a temporary
  location
- **THEN** on success, system replaces the installed basemap directory so that no partial
  installation becomes visible
- **THEN** system reloads the basemap in the rendering engine

#### Scenario: Installed metadata drives the decision

- **GIVEN** an installed basemap whose local metadata records a database format version and change
  time
- **WHEN** the system decides whether an update is available
- **THEN** it SHALL compare the server's version and change time against the local metadata
- **AND** it SHALL NOT need a directory listing or an archive file name to do so

#### Scenario: Update fails and the installed basemap stays

- **GIVEN** an installed basemap
- **WHEN** an update fails partway through
- **THEN** system SHALL keep the installed basemap usable
- **THEN** system SHALL report the failure to the user

### Requirement: Cancel basemap download

The system SHALL support cancelling an in-progress basemap download.

#### Scenario: Cancel during download
- **WHEN** user cancels basemap download
- **THEN** system stops the HTTP transfer
- **THEN** system removes any partial files
- **THEN** system SHALL NOT affect any previously installed basemap

### Requirement: Verify downloaded basemap files

The system SHALL verify each downloaded data file against the checksum the metadata names for it
before the basemap is registered, and SHALL discard the installation and report the error when a
file does not match.

#### Scenario: A file does not match its checksum

- **GIVEN** metadata naming a checksum for each data file
- **WHEN** a downloaded data file does not match its checksum
- **THEN** the installation SHALL be discarded
- **THEN** the failure SHALL be reported to the user
- **THEN** a previously installed basemap SHALL remain usable

#### Scenario: Every file matches

- **WHEN** all downloaded data files match the checksums named in the metadata
- **THEN** the basemap SHALL be registered with the map manager normally
