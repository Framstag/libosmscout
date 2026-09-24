# Spec Delta

## Purpose

Produces, versions, serves, and retires the world basemap in the map repository, so that clients
which already discover, download, and render a basemap as an overlay find a world-wide database
in the repository next to the regional databases.

## ADDED Requirements

### Requirement: Basemap configuration file

The regeneration pass SHALL take the basemap from a configuration file of its own, separate from
the imports manifest and the region index, and SHALL refuse to run when that file is missing or
does not satisfy the configuration contract. The imports manifest and the region index SHALL keep
their current meaning; the basemap SHALL NOT be declared as a region.

#### Scenario: Missing basemap configuration

- **GIVEN** a configuration area without a basemap configuration file
- **WHEN** a pass starts
- **THEN** it SHALL stop before any download or import, reporting the missing file
- **AND** no regional database SHALL be updated either

#### Scenario: Unsupported configuration version

- **GIVEN** a basemap configuration file carrying a version the pass does not support
- **WHEN** a pass starts
- **THEN** it SHALL report the mismatch and perform no work for the basemap

#### Scenario: Basemap is not a region

- **GIVEN** a basemap configuration file and a region index
- **WHEN** the configuration is validated
- **THEN** the basemap SHALL NOT be required to appear in the region index
- **AND** the manifest ids and region index leaves SHALL still have to match exactly

### Requirement: Declared inputs

The basemap configuration SHALL name the pre-filtered planet export as a path in the configuration
area, and SHALL allow the coastline source to be given as a location with an optional checksum.
When no coastline source is configured, the source the image carries SHALL apply. A named planet
export that cannot be read SHALL be reported as a configuration failure rather than as an import
failure.

#### Scenario: Planet export missing

- **GIVEN** a basemap configuration naming a planet export that does not exist
- **WHEN** the basemap step starts
- **THEN** it SHALL fail naming the path, before any import is attempted

#### Scenario: Coastline source defaults to the image value

- **GIVEN** a basemap configuration that does not name a coastline source
- **WHEN** the basemap step fetches coastline data
- **THEN** it SHALL use the location the image carries

#### Scenario: Configured coastline source wins

- **GIVEN** a basemap configuration naming its own coastline location
- **WHEN** the basemap step fetches coastline data
- **THEN** it SHALL use the configured location instead of the image value

### Requirement: Change detection and refresh cadence

The pass SHALL derive the basemap from the content of its inputs, not from their timestamps, and
SHALL produce a new basemap only when an input's content differs from the content the last placed
basemap was built from. Checks SHALL be gated by the configured refresh interval, so that a pass
inside the interval performs no check work for the basemap.

#### Scenario: Nothing changed inside the refresh interval

- **GIVEN** a basemap placed from the current input content
- **WHEN** a pass runs before the refresh interval has elapsed
- **THEN** it SHALL not check the inputs and SHALL not produce a new basemap

#### Scenario: Refresh interval elapsed, content unchanged

- **GIVEN** a basemap placed from the current input content
- **GIVEN** the refresh interval has elapsed
- **WHEN** a pass runs
- **THEN** it SHALL check the inputs, find the content unchanged, record the check, and produce no
  new basemap

#### Scenario: Planet export content changed

- **GIVEN** a planet export whose content differs from the recorded content
- **WHEN** the check runs
- **THEN** a new basemap SHALL be produced from it

### Requirement: Coastline acquisition and adoption cadence

The pass SHALL obtain coastline data from its configured or image-provided location, SHALL reuse a
previously fetched copy that the source reports as unchanged instead of transferring it again, and
SHALL record when it last checked and when it last adopted a copy. The pass SHALL adopt a newer
coastline copy, and with it produce a new basemap, only once the configured adoption interval has
elapsed, so that a source which is refreshed more often than the interval does not cause a basemap
regeneration on every pass. When the configuration provides a checksum for the coastline data, the
pass SHALL verify what it fetched against that checksum and SHALL NOT use data that does not match.
The adoption interval SHALL NOT be shorter than the refresh interval.

#### Scenario: Source reports no change

- **GIVEN** a fetchable coastline source and a previously fetched copy
- **WHEN** the source reports the remote data unchanged
- **THEN** the existing copy SHALL be reused and no bytes SHALL be transferred
- **AND** no new basemap SHALL be produced

#### Scenario: Source changed, adoption not yet due

- **GIVEN** a coastline source that has published newer data
- **GIVEN** the adoption interval has not elapsed since the last adoption
- **WHEN** the check runs
- **THEN** no new copy SHALL be adopted and no new basemap SHALL be produced
- **AND** the newer availability SHALL be reported in the pass output

#### Scenario: Source changed and adoption is due

- **GIVEN** a coastline source that has published newer data
- **GIVEN** the adoption interval has elapsed since the last adoption
- **WHEN** the check runs
- **THEN** the newer copy SHALL be adopted and a new basemap SHALL be produced from it

#### Scenario: Configured checksum does not match

- **GIVEN** a basemap configuration carrying a checksum for the coastline data
- **WHEN** the fetched coastline data does not match that checksum
- **THEN** the fetched data SHALL be discarded
- **AND** the failure SHALL be reported
- **AND** the basemap already served SHALL remain served unchanged

#### Scenario: Adoption interval shorter than refresh interval

- **GIVEN** a basemap configuration whose adoption interval is shorter than its refresh interval
- **WHEN** the configuration is validated
- **THEN** validation SHALL fail, naming the two intervals

### Requirement: Basemap content

A generated basemap SHALL be a database built from the configured planet export using the type
definitions the image bundles for basemaps, and SHALL contain a water index derived from the
coastline data rather than from the planet export. The basemap SHALL carry the same metadata as a
regional database: the database format version, the generation time, and a checksum for every data
file it exposes, including the water index. The basemap SHALL contain every file a client requires
of a database directory.

#### Scenario: Metadata is present and complete

- **WHEN** a basemap has been produced
- **THEN** its metadata SHALL name the database format version and the generation time
- **AND** SHALL carry a checksum for every data file the basemap exposes

#### Scenario: Water index comes from the coastline data

- **GIVEN** a basemap produced from a planet export that itself contains coastline geometry
- **WHEN** the basemap's water index is compared with the configured coastline data
- **THEN** the water index SHALL be the one derived from the coastline data

#### Scenario: File set a client requires

- **WHEN** a basemap has been produced
- **THEN** every file a client requires of a database directory SHALL exist in it, even when the
  basemap contains no objects for it

### Requirement: Version-keyed slot placement

The basemap SHALL be placed in a directory keyed by the database format version of the produced
database, inside the served part of the repository, and SHALL be placed by atomic replacement so
that a pass interrupted mid-placement leaves the previously served basemap intact and never exposes
a partially written directory or a partially written file set.

#### Scenario: New database format version

- **GIVEN** a served basemap of one database format version
- **WHEN** a produced basemap carries a different database format version
- **THEN** it SHALL be placed in a directory of its own
- **AND** the previously served version SHALL remain served until retention removes it

#### Scenario: Interrupted placement

- **GIVEN** a basemap being placed
- **WHEN** the pass is interrupted between removing the previous content and moving in the new
- **THEN** a later pass SHALL restore or complete the served content so that the served basemap is
  again complete

#### Scenario: Same version produced again

- **GIVEN** a served basemap of a given database format version
- **WHEN** a new basemap of the same version is produced
- **THEN** it SHALL replace the served content of that version without exposing a mixed file set

### Requirement: Availability manifest

After a basemap has been placed, the pass SHALL write a manifest in the served basemap area that
names the database format versions currently served and when each of them last changed. The
manifest SHALL be written as a whole, and only after the basemap it describes has been placed.

#### Scenario: Manifest lists what is served

- **GIVEN** a repository serving basemaps of one or more database format versions
- **WHEN** the manifest is read
- **THEN** it SHALL name each served version and the time of its last change

#### Scenario: Manifest is written last

- **GIVEN** a basemap that has been produced but not yet placed
- **WHEN** the placement fails
- **THEN** the manifest SHALL NOT name that basemap

#### Scenario: Manifest is never partially visible

- **WHEN** the manifest is written
- **THEN** a reader SHALL see either the previous manifest or the complete new one

### Requirement: Retention

The basemap configuration SHALL define how many basemap versions the server keeps, and the pass
SHALL remove older versions beyond that number after a successful placement. A retention value
that asks for no pruning SHALL keep all versions.

#### Scenario: Older versions are pruned

- **GIVEN** a retention of two versions and three served basemap versions
- **WHEN** a basemap has been placed successfully
- **THEN** the oldest version SHALL be removed and the newest two SHALL remain served

#### Scenario: No pruning configured

- **GIVEN** a retention value that asks for no pruning
- **WHEN** basemaps are placed repeatedly
- **THEN** earlier versions SHALL remain served

#### Scenario: Pruning happens after placement

- **GIVEN** a placement that fails
- **WHEN** the pass ends
- **THEN** no previously served version SHALL have been removed

### Requirement: Records of the basemap step

The pass SHALL keep, in the area that is not served, a record of the input content currently
served, of when the coastline source was last checked and last adopted, and a record for each
placed basemap naming its version and when it was placed. These records SHALL be sufficient to
explain what the last pass decided and to decide the next one.

#### Scenario: Check state reflects what is served

- **GIVEN** a pass that placed a basemap
- **WHEN** the check state is read
- **THEN** it SHALL name the input content that basemap was built from
- **AND** it SHALL name when the coastline source was last checked and last adopted

#### Scenario: Generation record per placement

- **GIVEN** a basemap that has been placed
- **WHEN** the non-served area is inspected
- **THEN** it SHALL contain a record naming the placed version, the time, and which versions were
  pruned by that placement

#### Scenario: Unchanged pass leaves the records alone

- **GIVEN** a pass that found no input change
- **WHEN** it ends
- **THEN** the check state SHALL be updated with the check, and no generation record SHALL be added

### Requirement: Failure isolation

A basemap step that fails SHALL be reported as a failure of the pass, and SHALL NOT prevent the
regional imports of the same pass from completing, nor disturb a basemap that is already served.

#### Scenario: Basemap production fails, regions succeed

- **GIVEN** a pass with due regional imports and a failing basemap step
- **WHEN** the pass ends
- **THEN** the regional imports SHALL have been performed
- **AND** the pass SHALL exit reporting the basemap failure

#### Scenario: Previously served basemap survives a failure

- **GIVEN** a served basemap
- **WHEN** the basemap step fails
- **THEN** the served basemap SHALL still be served, unchanged
