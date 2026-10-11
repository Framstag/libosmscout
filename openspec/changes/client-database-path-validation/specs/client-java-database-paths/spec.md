# Spec Delta

## Purpose

Let a Java client open a whole list of map database directories in one call, with a per-input result, so
that registering a list costs one database-set change instead of one per directory.

## ADDED Requirements

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
