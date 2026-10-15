# Spec Delta

## Purpose

Define how a client scans its registered lookup directories for map databases: the scan discovers the
databases, publishes one complete set per scan, works on the directories registered when it starts, and can
be stopped as part of closing the client that owns it.

## ADDED Requirements

### Requirement: A scan publishes one complete set of found databases

A scan of the registered lookup directories SHALL discover the map databases below those directories and
publish the complete set of what it found in a single publication, replacing the previously published set.
A scan SHALL NOT publish a partially updated set. A database reachable through more than one registered
directory SHALL appear once in the published set. A scan that finds no database SHALL still publish an empty
set and notify its listeners.

#### Scenario: Databases from several directories are published together

- **GIVEN** several registered directories each containing a map database
- **WHEN** a scan completes
- **THEN** the published set SHALL contain every one of those databases
- **AND** listeners SHALL be notified once, with the complete set

#### Scenario: A database reachable through two directories appears once

- **GIVEN** two registered directories below which the same map database is reachable
- **WHEN** a scan completes
- **THEN** the published set SHALL contain that database exactly once

#### Scenario: A scan without a database publishes an empty set

- **GIVEN** registered directories that contain no map database
- **WHEN** a scan completes
- **THEN** the published set SHALL be empty
- **AND** listeners SHALL still be notified

#### Scenario: A directory that is not a readable directory does not end the scan

- **GIVEN** a registered directory that does not exist, is not a directory, or cannot be read
- **WHEN** a scan runs
- **THEN** the scan SHALL continue with the remaining registered directories
- **AND** it SHALL still publish the databases it found

### Requirement: A scan works on the directories registered when it starts

A scan SHALL operate on the set of registered directories as it is when the scan starts, independent of the
manager's own storage, so that a registration during the scan cannot change or invalidate the directories the
scan is working on.

#### Scenario: A directory registered during a scan is not part of that scan

- **GIVEN** a running scan
- **WHEN** a further directory is registered before the scan completes
- **THEN** the publication of that scan SHALL NOT depend on the newly registered directory
- **AND** a later scan SHALL include it

#### Scenario: A running scan keeps working after a concurrent registration

- **GIVEN** a scan of a large directory tree in progress
- **WHEN** a directory is registered at the same time
- **THEN** the scan SHALL complete without reading released or moved storage
- **AND** no fault SHALL occur

### Requirement: A scan never outlives the manager that owns it

Destroying the manager that owns the scan SHALL stop the scan and SHALL NOT wait for it to run to completion.
No publication SHALL be made on behalf of a scan that was stopped, and destroying the manager while a scan
runs SHALL NOT end the process.

#### Scenario: Closing the manager while a scan runs

- **GIVEN** a manager whose scan of a large directory tree is in progress
- **WHEN** the manager is destroyed
- **THEN** the scan SHALL stop at its next check
- **AND** destruction SHALL return without waiting for the remaining directories
- **AND** the process SHALL continue

#### Scenario: Stopping during the walk of a single large directory

- **GIVEN** a manager whose scan is walking one registered directory that holds many entries
- **WHEN** the manager is destroyed while that walk is in progress
- **THEN** the scan SHALL stop while walking that directory, without visiting its remaining entries
- **AND** destruction SHALL return without waiting for the walk to finish

#### Scenario: A stopped scan publishes nothing

- **GIVEN** a manager whose scan was stopped by its destruction
- **WHEN** the scan stops
- **THEN** it SHALL NOT publish a set of found databases
- **AND** listeners SHALL NOT be notified

#### Scenario: Destroying the manager before the scan starts

- **GIVEN** a manager with a scan that has been requested but has not started
- **WHEN** the manager is destroyed
- **THEN** the scan SHALL NOT run
- **AND** no set SHALL be published

#### Scenario: Repeated scans around teardown are safe

- **GIVEN** a client that requests a scan and is then destroyed, repeatedly
- **WHEN** each destruction happens while its scan may be running
- **THEN** every destruction SHALL complete without a fault
