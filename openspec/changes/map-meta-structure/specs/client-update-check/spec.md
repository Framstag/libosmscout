## Purpose

Client-side update detection: a client learns what databases are available for its own type-config version, compares their creation timestamps with its local state, and can download and verify a database without ever probing newer type-config versions.

## ADDED Requirements

### Requirement: Own-version probe only
A client SHALL look for database metadata only under the directory of its own type-config version. The client SHALL NOT probe directories of newer type-config versions.

#### Scenario: Client probes own version
- **WHEN** a client checks for available data
- **THEN** it requests the metadata of its own type-config version only

#### Scenario: Newer version ignored
- **WHEN** a server holds a database of a newer type-config version than the client's
- **THEN** the client performs no request for it and does not offer it to the user

### Requirement: Update comparison
A client SHALL compare the creation timestamp in the server database metadata with the creation timestamp of its locally stored database of the same import. A newer server timestamp SHALL be reported as an available update; an equal or older one SHALL be reported as up to date.

#### Scenario: Newer data available
- **WHEN** the server database metadata timestamp is newer than the local database timestamp
- **THEN** the client reports that an update is available

#### Scenario: Up to date
- **WHEN** the server database metadata timestamp is not newer than the local database timestamp
- **THEN** the client reports the local data as up to date

### Requirement: Missing data for own version
When no database metadata exists for the client's type-config version, the client SHALL treat the data as unavailable, keep its existing local state, and stop the check for that import.

#### Scenario: No metadata for own version
- **WHEN** the server has no database metadata for the client's type-config version
- **THEN** the client treats the data as unavailable and reports that no update check result for it

### Requirement: Fresh install offer
When a client has no local database for an import and the server has a database for the client's type-config version, the client SHALL present that database as available for installation.

#### Scenario: Database offered to fresh client
- **WHEN** a client without local data finds server metadata for its own type-config version
- **THEN** the client presents the database as available for download

### Requirement: Downloaded file verification
A client SHALL verify downloaded database files against the integrity checksums in the database metadata and SHALL treat a verification failure as a failed download, not as usable data.

#### Scenario: Corrupt download detected
- **WHEN** a downloaded database file does not match its checksum in the metadata
- **THEN** the client rejects the download and does not use the corrupted file

### Requirement: Client-accessible data
The data a client can access SHALL be limited to the region index, the per-database metadata files, and the database files themselves. Run records and check-cycle state of the regeneration process SHALL NOT be accessible to clients.

#### Scenario: Run records inaccessible
- **WHEN** a client requests server data
- **THEN** it can at most receive region index, database metadata, and database files
