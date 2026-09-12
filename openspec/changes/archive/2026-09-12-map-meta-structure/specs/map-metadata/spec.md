## Purpose

Per-database metadata file emitted by the import tool at the end of a successful import run, carrying everything a client or the regeneration process needs to verify and manage a generated database.

## ADDED Requirements

### Requirement: Metadata emission
The import tool SHALL write a metadata file named `db.json` into the destination directory when an import run completes successfully. It SHALL NOT write the metadata file when the run fails.

#### Scenario: Successful import writes metadata
- **WHEN** an import run completes without errors
- **THEN** the destination directory contains `db.json` describing the generated database

#### Scenario: Failed import writes no metadata
- **WHEN** an import run fails
- **THEN** no `db.json` is written and the failure is reported

### Requirement: Metadata content
The metadata file SHALL contain the creation timestamp of the database, the type-config version, the import tool identity and version, the executed import steps, the run duration, and a schema version.

#### Scenario: All identity fields present
- **WHEN** a client reads a metadata file from a successful import
- **THEN** it can read creation timestamp, type-config version, tool identity, steps, duration, and schema version from it

### Requirement: Source identification
The import tool SHALL accept the source URL and the integrity hash of the source data as run parameters and SHALL record them verbatim in the metadata file.

#### Scenario: Source facts recorded
- **WHEN** an import runs with source URL and source hash parameters
- **THEN** the metadata file contains exactly those values

#### Scenario: Source parameters optional
- **WHEN** an import runs without source parameters
- **THEN** the import still succeeds and the metadata omits source identification

### Requirement: Output file inventory
The metadata file SHALL list every output file placed in the destination directory, with its byte size and an integrity checksum, such that a consumer can detect a corrupted or truncated file. The inventory SHALL match the actual file set written by the import.

#### Scenario: Inventory matches files on disk
- **WHEN** a consumer compares the metadata inventory with the destination directory
- **THEN** every listed file exists with the listed size and checksum, and no generated data file is missing from the inventory

#### Scenario: Corruption detectable
- **WHEN** a consumer verifies a downloaded file against its checksum and the file content differs
- **THEN** the verification fails and the consumer treats the database as corrupt

### Requirement: Bounding box and statistics
The metadata file SHALL contain the bounding box of the imported data and basic statistics of the generated database (at minimum the number of defined types).

#### Scenario: Bounding box present
- **WHEN** a client reads a metadata file
- **THEN** it can read the bounding box of the imported data

### Requirement: Relative references
The metadata file SHALL reference output files and all other data by relative names only. It SHALL NOT contain absolute paths or host-specific information, so the file can be copied verbatim together with the database.

#### Scenario: Metadata copyable
- **WHEN** the database with its metadata file is copied to another directory or host
- **THEN** the metadata file remains valid without modification
