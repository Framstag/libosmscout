# Spec Delta

## MODIFIED Requirements

### Requirement: Failure handling
The regeneration process SHALL report failures per import, SHALL NOT leave a partially placed database, and SHALL exit with a non-zero status when any import failed. A failure SHALL be recoverable without an operator: a download SHALL be retried within the same run, bounded, before the import counts as failed; downloads and hash fetches SHALL have timeouts, including an idle-transfer timeout, so a stalled transfer ends the attempt instead of holding the run; and a download that fails verification SHALL be discarded rather than kept or resumed from. A source that an earlier run left in the work area SHALL be resumed only while it belongs to the source that is being fetched - a partial transfer of that source - and SHALL be discarded before the transfer otherwise, so no part of another source can enter the download.

#### Scenario: Partial failure
- **WHEN** one import fails while another succeeds
- **THEN** the successful database is placed, the failure is recorded, no corrupt partial state is left, and the process exits with an error status

#### Scenario: A failed download is retried
- **WHEN** a download fails while the source is reachable
- **THEN** the process attempts it again within the same run before recording the import as failed

#### Scenario: A stalled transfer ends the attempt
- **WHEN** a transfer stops delivering data
- **THEN** the attempt ends through its idle timeout, the run reports it and does not hold the lock further

#### Scenario: An unverified download is discarded
- **WHEN** a downloaded source does not match the published hash
- **THEN** it is removed, no import runs on it, and a later attempt starts from a fresh download

#### Scenario: A partial download is resumed
- **GIVEN** a source file in the work area that a run left behind and that belongs to the source being fetched
- **WHEN** that source is fetched again
- **THEN** the transfer continues from what was already downloaded, and the result is verified before use

#### Scenario: A source kept from an earlier check is discarded
- **GIVEN** a source file in the work area that belongs to a source other than the one being fetched
- **WHEN** that source is fetched
- **THEN** the kept file SHALL be discarded before the transfer starts
- **THEN** the fetched result SHALL match the published hash, and no report of a corrupt download SHALL be produced for it

### Requirement: Recovery from interrupted work
The regeneration process SHALL recover from work that an earlier run was interrupted in the middle of, without an operator. An interrupted replacement SHALL be finished or rolled back by the next run, so the served tree always holds either the previous database or the new one. A successful run SHALL leave neither the downloaded source nor the import output behind, so the work area does not grow with the number of runs, while a failed run keeps what a later attempt can resume or rebuild - and SHALL keep with it the record of which source it belongs to, so that a later run can tell it apart from the source of an earlier check.

#### Scenario: An interrupted replacement is rolled back
- **GIVEN** a run that was killed between moving the served database aside and moving the new one in
- **WHEN** the next run starts
- **THEN** the previous database SHALL be served again, or the new one placed, and no other outcome SHALL be visible to a reader

#### Scenario: A successful run leaves the work area clean
- **WHEN** an import is imported and placed
- **THEN** the downloaded source and the import output of that import SHALL be gone from the work area

#### Scenario: A failed run keeps what a retry can use
- **WHEN** an import failed after its source was downloaded
- **THEN** the source SHALL remain for a later attempt to resume, and later runs SHALL NOT accumulate further copies of it

#### Scenario: What a kept file belongs to is recoverable
- **WHEN** a run leaves a source file in the work area, whether by an interrupted transfer or by a failed import
- **THEN** a later run can decide from the work area alone whether that file is a partial of the source it is about to fetch
- **THEN** it resumes the file only in that case and discards it otherwise
