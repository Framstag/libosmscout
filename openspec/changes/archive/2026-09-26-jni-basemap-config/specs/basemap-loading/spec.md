# Spec Delta

## ADDED Requirements

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
