# Spec Delta

## MODIFIED Requirements

### Requirement: Check-cycle state
The regeneration process SHALL persist the last check time per import so that refresh frequency gating is stable across runs. It SHALL record that state when the check has been completed - the source was found unchanged, or it was imported and placed - and SHALL NOT record it for an attempt that failed, so that a failed import stays due and is retried by the next run without waiting for the refresh window.

#### Scenario: Check time persisted
- **WHEN** an import is checked and the source turns out to be unchanged
- **THEN** the check time is stored and used by subsequent runs for the refresh gate

#### Scenario: A completed import records the check
- **WHEN** an import is downloaded, imported and placed
- **THEN** the check time and the source hash are stored, and the next run applies the refresh gate to them

#### Scenario: A failed import stays due
- **WHEN** the download or the import of an import fails
- **THEN** the state of that import is left as it was, so the next run treats it as due and retries it

### Requirement: Failure handling
The regeneration process SHALL report failures per import, SHALL NOT leave a partially placed database, and SHALL exit with a non-zero status when any import failed. A failure SHALL be recoverable without an operator: a download SHALL be retried within the same run, bounded, before the import counts as failed; downloads and hash fetches SHALL have timeouts, including an idle-transfer timeout, so a stalled transfer ends the attempt instead of holding the run; and a download that fails verification SHALL be discarded rather than kept or resumed from.

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
- **WHEN** a download was interrupted and the source is fetched again
- **THEN** the transfer continues from what was already downloaded, and the result is verified before use

## ADDED Requirements

### Requirement: Recovery from interrupted work

The regeneration process SHALL recover from work that an earlier run was interrupted in the middle of, without
an operator. An interrupted replacement SHALL be finished or rolled back by the next run, so the served tree
always holds either the previous database or the new one. A successful run SHALL leave neither the downloaded
source nor the import output behind, so the work area does not grow with the number of runs, while a failed run
keeps what a later attempt can resume or rebuild.

#### Scenario: An interrupted replacement is rolled back
- **GIVEN** a run that was killed between moving the served database aside and moving the new one in
- **WHEN** the next run starts
- **THEN** the previous database SHALL be served again, or the new one placed, and no other outcome SHALL be
  visible to a reader

#### Scenario: A successful run leaves the work area clean
- **WHEN** an import is imported and placed
- **THEN** the downloaded source and the import output of that import SHALL be gone from the work area

#### Scenario: A failed run keeps what a retry can use
- **WHEN** an import failed after its source was downloaded
- **THEN** the source SHALL remain for a later attempt to resume, and later runs SHALL NOT accumulate further
  copies of it
