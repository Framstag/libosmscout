## Purpose

Machine-readable registry of all supported map imports, defining download sources, refresh frequency, and retention depth; the base input for the regeneration process.

## ADDED Requirements

### Requirement: Imports manifest structure
The system SHALL maintain an imports manifest that lists every supported import, each with a unique id and a download source URL. The manifest SHALL carry a schema version so consumers can detect format changes.

#### Scenario: Manifest with single import
- **WHEN** the manifest contains one import with id and download source
- **THEN** the regeneration process can resolve the id and the source URL for that import

#### Scenario: Unknown manifest schema
- **WHEN** the manifest carries a schema version the regeneration process does not support
- **THEN** the regeneration process refuses to run and reports the mismatch

### Requirement: Unique import ids
The ids in the imports manifest SHALL be unique. Every id SHALL be usable as a leaf reference by the region index.

#### Scenario: Duplicate id
- **WHEN** two imports in the manifest share the same id
- **THEN** the manifest is invalid and the regeneration process refuses to run

### Requirement: Refresh frequency
The imports manifest SHALL define a refresh frequency as a global default and SHALL allow per-id overrides. The refresh frequency SHALL gate how often the regeneration process checks a given import for changes.

#### Scenario: Global refresh default applies
- **WHEN** an import has no explicit refresh frequency
- **THEN** the global default refresh frequency applies to that import

#### Scenario: Per-id refresh override
- **WHEN** an import declares its own refresh frequency
- **THEN** that value overrides the global default for this import only

### Requirement: Retention depth
The imports manifest SHALL define a retention depth as a global default and SHALL allow per-id overrides. The retention depth SHALL state how many type-config versions of a database the server keeps for an import.

#### Scenario: Global retention default applies
- **WHEN** an import has no explicit retention depth
- **THEN** the global default retention depth applies to that import

#### Scenario: Per-id retention override
- **WHEN** an import declares its own retention depth
- **THEN** that value overrides the global default for this import only

### Requirement: Manifest validation
The regeneration process SHALL validate the imports manifest before any download or import work, rejecting manifests with unknown schema versions, duplicate ids, or malformed entries.

#### Scenario: Malformed manifest rejected
- **WHEN** the manifest contains a malformed entry
- **THEN** the regeneration process reports the error and performs no downloads and no imports
