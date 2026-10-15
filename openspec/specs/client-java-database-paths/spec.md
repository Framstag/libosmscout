# client-java-database-paths Specification

## Purpose
Let a Java client open a whole list of map database directories in one call, with a per-input result, so
that registering a list costs one database-set change instead of one per directory.

## Requirements

### Requirement: The Java client can open a list of database directories in one call

The system SHALL provide a native Java method `boolean[] openDatabases(String[] paths)` on
`OSMScoutClient`. The method SHALL register the whole list as one operation and SHALL ask the database
thread to process the registered set once, not once per directory. The returned array SHALL be
index-aligned with the input: the entry at position `i` SHALL be true when the directory at input position
`i` is part of the registered set after the call, and false otherwise.

#### Scenario: A list of directories is registered in one call

- **GIVEN** several map database directories that are not registered yet
- **WHEN** `openDatabases` is called with that list
- **THEN** the returned array SHALL have the length of the input
- **AND** every entry SHALL be true
- **AND** the client's registered set SHALL contain all of them

#### Scenario: The per-input result distinguishes what was registered

- **GIVEN** a client with one of the requested directories already registered
- **WHEN** `openDatabases` is called with a list that contains that directory, a new directory and a null
  element
- **THEN** the entry for the already registered directory SHALL be true
- **AND** the entry for the new directory SHALL be true
- **AND** the entry for the null element SHALL be false

#### Scenario: A null or empty array is accepted

- **WHEN** `openDatabases` is called with a null array or an empty array
- **THEN** the call SHALL return an empty array
- **AND** no database-set change SHALL be requested

#### Scenario: An unusable client reports failure instead of faulting

- **GIVEN** a client whose database thread is not available
- **WHEN** `openDatabases` is called with a list of directories
- **THEN** the call SHALL return an array in which every entry is false
- **AND** it SHALL NOT fault

#### Scenario: The registered set is shared with the single-directory call

- **GIVEN** a client with directories registered through `openDatabase`
- **WHEN** `openDatabases` is called afterwards
- **THEN** the registered set SHALL contain the directories of both calls

### Requirement: The database thread receives a complete set

The native side SHALL hand the database thread a value snapshot of the registered set, taken under the lock
that protects it and released before the database thread is called. Concurrent calls from several threads
SHALL NOT be able to make the database thread read a list that another caller is mutating.

#### Scenario: A concurrent opener cannot invalidate the published set

- **GIVEN** one thread that registers a list of directories
- **WHEN** another thread registers a further directory at the same time
- **THEN** the set handed to the database thread SHALL be one of the complete sets, not a partially updated
  list
- **AND** no call SHALL fault

#### Scenario: The single-directory call publishes a snapshot too

- **WHEN** `openDatabase` is called
- **THEN** the directory SHALL be registered in the shared set
- **AND** the database thread SHALL receive a snapshot of the complete set

### Requirement: The single-directory call rejects a path that is not a database directory

The Java client's single-directory call SHALL refuse a path that does not exist or is not a directory, SHALL
NOT register it and SHALL NOT ask the database thread to process a set change for it. It SHALL report the
refusal to the caller and SHALL leave every database already open in place. The batch call SHALL keep
registering the paths it was handed, whatever the filesystem says about them, and SHALL report a path that
is not a directory without changing its entry.

#### Scenario: A directory is opened

- **GIVEN** an existing map database directory
- **WHEN** the single-directory call is made with it
- **THEN** the call SHALL report success
- **AND** the directory SHALL be part of the registered set

#### Scenario: A path that is not a directory is refused

- **GIVEN** a path that does not exist or is a regular file
- **WHEN** the single-directory call is made with it
- **THEN** the call SHALL report failure
- **AND** the path SHALL NOT be part of the registered set
- **AND** no database-set change SHALL be requested

#### Scenario: A refusal keeps the loaded databases open

- **GIVEN** a client with a directory already registered and its databases open
- **WHEN** the single-directory call is made with a path that is not a directory
- **THEN** the call SHALL report failure
- **AND** the already registered directory SHALL still be part of the set
- **AND** the database thread SHALL NOT be asked to process a new set

#### Scenario: The batch call keeps its tolerance

- **GIVEN** a list of directories that contains a path which is not a directory
- **WHEN** the batch call is made with that list
- **THEN** the entry for that path SHALL be true, the batch having registered it
- **AND** the batch SHALL complete without reporting failure for the list
- **AND** a report about that path SHALL NOT change the entry the batch returns
