# Spec Delta

## Purpose

Own the set of map database directories a client has been asked to open, register paths into it safely
from any thread, publish the complete set as a value so that other threads never read a list that is being
mutated, and make the number of database-set changes observable.

## ADDED Requirements

### Requirement: A path can be registered only when it is an openable directory

The registry SHALL offer a registration entry point that accepts a path only when the path exists and is a
directory, and reports whether the path is part of the registered set afterwards. Any filesystem error
SHALL be answered as "not openable" without raising. A symbolic link to a directory SHALL count as a
directory.

#### Scenario: An existing directory is accepted

- **GIVEN** an existing directory
- **WHEN** it is registered through the validating entry point
- **THEN** the entry point SHALL report true
- **AND** the set SHALL contain the path

#### Scenario: A missing path is rejected

- **GIVEN** a path that does not exist
- **WHEN** it is registered through the validating entry point
- **THEN** the entry point SHALL report false
- **AND** the set SHALL NOT contain the path

#### Scenario: A regular file is rejected

- **GIVEN** an existing regular file
- **WHEN** it is registered through the validating entry point
- **THEN** the entry point SHALL report false
- **AND** the set SHALL NOT contain the path

#### Scenario: A permission failure is answered without raising

- **GIVEN** a directory that cannot be searched
- **WHEN** it is registered through the validating entry point
- **THEN** the entry point SHALL report false
- **AND** SHALL NOT raise

#### Scenario: A rejected path counts no set change

- **GIVEN** a registry with a recorded number of set changes
- **WHEN** a missing path is registered through the validating entry point
- **THEN** the number of set changes SHALL be unchanged

### Requirement: The batch registration never lets the filesystem decide

The validating entry point SHALL be the only registration entry point that can refuse a path, and the
predicate SHALL be the registry's only path check. The list entry point SHALL register every path it was
handed, whatever the filesystem says about that path, so a directory that disappears between a scan and the
call cannot fail the batch. A report about a path of the list SHALL NOT change what the list registers.

#### Scenario: A batch registers a path that is not a directory

- **GIVEN** a list that contains a path which is not an existing directory
- **WHEN** the list is registered
- **THEN** the path SHALL be part of the set afterwards
- **AND** the call SHALL NOT report failure for the list

#### Scenario: A report about a path of the batch changes nothing

- **GIVEN** a list with a path that is not an existing directory, and a registry that counts set changes
- **WHEN** the predicate is asked about that path and the list is registered
- **THEN** the path SHALL be part of the set afterwards
- **AND** the disposition reported for that input SHALL be true
- **AND** the number of set changes SHALL have grown by exactly one

#### Scenario: A path that disappeared after a scan does not fail the batch

- **GIVEN** a list of directories of which one was removed after it was scanned
- **WHEN** the list is registered
- **THEN** the call SHALL complete with one set change
- **AND** the remaining directories SHALL be part of the set
