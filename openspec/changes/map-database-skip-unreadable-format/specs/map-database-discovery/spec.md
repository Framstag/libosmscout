## ADDED Requirements

### Requirement: A database this build cannot read is left out of the scan

The scan SHALL read each candidate directory's database data-format version from its `types.dat` and
SHALL leave the directory out of the published set when that version is present and differs from the
version this build reads. The scan SHALL log one warning naming the directory and both versions, and
SHALL count the directory as invalid in its per-directory summary.

#### Scenario: An older data format is skipped

- **GIVEN** a lookup directory holding a valid database whose `types.dat` names the data-format
  version one below the version this build reads
- **WHEN** the scan runs
- **THEN** that directory SHALL NOT be part of the published database set

#### Scenario: A newer data format is skipped

- **GIVEN** a lookup directory holding a valid database whose `types.dat` names the data-format
  version one above the version this build reads
- **WHEN** the scan runs
- **THEN** that directory SHALL NOT be part of the published database set

#### Scenario: A database this build reads stays published

- **GIVEN** a lookup directory holding a valid database whose `types.dat` names exactly the
  data-format version this build reads
- **WHEN** the scan runs
- **THEN** that directory SHALL be part of the published database set

#### Scenario: The skip is named

- **GIVEN** a candidate directory whose data-format version is not the one this build reads
- **WHEN** the scan leaves it out
- **THEN** the scan SHALL emit a warning naming the directory and both versions

### Requirement: A version the scan cannot read decides nothing

When a candidate directory's data-format version cannot be read — an empty or truncated `types.dat`,
or one the file read rejects — the scan SHALL keep that directory a candidate and SHALL offer it to
the reader through the existing validity check. The scan SHALL NOT skip such a directory and SHALL
NOT warn about its version, because it cannot judge it and the reader stays the authority.

#### Scenario: An unreadable version stays published

- **GIVEN** a lookup directory holding a valid database whose `types.dat` is shorter than the
  data-format version field, beside another valid database
- **WHEN** the scan runs
- **THEN** both directories SHALL be part of the published database set

#### Scenario: The reader remains the authority

- **GIVEN** a candidate directory whose data-format version the scan cannot read
- **WHEN** the scan admits it and the reader later opens it
- **THEN** the reader SHALL be the one to accept or refuse that database
