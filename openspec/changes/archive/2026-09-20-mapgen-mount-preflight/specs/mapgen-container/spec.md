# Spec Delta

## ADDED Requirements

### Requirement: An unwritable area is reported, not discovered later

Before running the pass, in either mode, the image SHALL verify that the areas it has to write to - the
transient work area and the database repository - are writable by the identity the process runs as. When an
area is not writable, the image SHALL refuse to start and SHALL report the identity in use, the area that
failed and how to give the area that ownership, instead of failing at the first write with the error of that
write. A run whose arguments request a read-only operation, such as a configuration check, SHALL NOT be
blocked by this check.

#### Scenario: A work area owned by another identity

- **GIVEN** a container started with a work area the process may not write to
- **WHEN** it starts
- **THEN** it SHALL stop with a status reporting the failure
- **AND** the report SHALL name the work area, the uid and gid in use, and a command that gives the area
  that ownership
- **AND** no pass SHALL run

#### Scenario: A repository owned by another identity

- **GIVEN** a container started with a writable work area and a repository the process may not write to
- **WHEN** it starts
- **THEN** it SHALL stop with a status reporting the failure, naming the repository area in the same way

#### Scenario: A read-only run is not blocked

- **GIVEN** a container started with arguments that ask for a read-only operation
- **WHEN** the areas are owned by another identity
- **THEN** the operation SHALL be performed and its result reported, and the check SHALL NOT stop it

#### Scenario: Writable areas start normally

- **GIVEN** a container whose work area and repository are writable by the identity it runs as
- **WHEN** it starts, in either mode
- **THEN** it SHALL run as before, and the check SHALL leave no trace behind in the areas
