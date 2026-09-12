## Purpose

Regeneration process that checks each import for changes, verifies and downloads new source data, runs the import tool, places the resulting databases at their defined server positions, and prunes old versions, keeping a script-owned record of every run.

## ADDED Requirements

### Requirement: Check cycle gating
The regeneration process SHALL check each import for changes only when the time since the last check exceeds the import's refresh frequency (global default or per-id override). Imports that are not due SHALL be skipped without any network activity.

#### Scenario: Import not due is skipped
- **WHEN** an import was checked less than its refresh frequency ago
- **THEN** the regeneration process skips it without downloading anything

#### Scenario: Import due is checked
- **WHEN** an import's last check is older than its refresh frequency
- **THEN** the regeneration process performs the change check for it

### Requirement: Source change detection
The regeneration process SHALL detect whether the source data of an import changed since the last import, comparing the source integrity hash published by the download service with the hash recorded in the newest stored database metadata. Imports with unchanged source SHALL be skipped.

#### Scenario: Unchanged source skipped
- **WHEN** the published source hash equals the hash in the newest database metadata
- **THEN** the regeneration process performs no download and no import for that import

#### Scenario: Changed source triggers import
- **WHEN** the published source hash differs from the newest database metadata hash
- **THEN** the regeneration process verifies and downloads the new source data and runs the import

### Requirement: Source verification
The regeneration process SHALL verify the integrity of downloaded source data against the hash published by the download service before running the import. Data that fails verification SHALL NOT be imported.

#### Scenario: Verification failure aborts import
- **WHEN** the downloaded source data does not match the published hash
- **THEN** no import runs and the failure is recorded

### Requirement: Import invocation
The regeneration process SHALL invoke the import tool with the source URL and the verified source hash, so the generated database metadata records the provenance.

#### Scenario: Provenance passed to import
- **WHEN** the regeneration process runs the import tool
- **THEN** the import tool receives the source URL and the verified source hash

### Requirement: Placement
The regeneration process SHALL place the generated database (data files and metadata file) at the server position implied by the region index tree, under a directory keyed by the type-config version recorded in the database metadata. Placement SHALL only happen when the database was generated successfully.

#### Scenario: Database placed at defined position
- **WHEN** an import succeeds
- **THEN** the database lands at the region index position of its import id, below a directory for its type-config version

#### Scenario: Failed import not placed
- **WHEN** an import fails
- **THEN** no database is placed for that import

### Requirement: Atomic replacement
Replacing an existing database in a version slot SHALL NOT expose readers to a partially written database. A read of the slot SHALL either see the complete previous database or the complete new database, never a mixture.

#### Scenario: Reader during replacement
- **WHEN** a new database replaces an existing one in a version slot
- **THEN** any reader of that slot sees either the complete old database or the complete new database

### Requirement: Pruning
After a successful placement, the regeneration process SHALL remove older type-config versions of that import until at most the retention depth remains, keeping the newest versions.

#### Scenario: Versions pruned to retention depth
- **WHEN** an import keeps more type-config versions than its retention depth allows
- **THEN** the oldest versions are removed until only the newest ones within the retention depth remain

### Requirement: Run records
The regeneration process SHALL maintain a script-owned record per placed database, documenting placement time, server position, retention used, pruned versions, run outcome, and the change detection result. These records SHALL be kept outside the client-accessible data.

#### Scenario: Record written per placement
- **WHEN** a database is placed
- **THEN** a record exists documenting the placement and run details

#### Scenario: Client cannot read records
- **WHEN** a client requests data from the server
- **THEN** the run records are never part of the response

### Requirement: Check-cycle state
The regeneration process SHALL persist the last check time per import so that refresh frequency gating is stable across runs.

#### Scenario: Check time persisted
- **WHEN** an import is checked
- **THEN** the check time is stored and used by subsequent runs for the refresh gate

### Requirement: Failure handling
The regeneration process SHALL report failures per import, SHALL NOT leave a partially placed database, and SHALL exit with a non-zero status when any import failed.

#### Scenario: Partial failure
- **WHEN** one import fails while another succeeds
- **THEN** the successful database is placed, the failure is recorded, no corrupt partial state is left, and the process exits with an error status
