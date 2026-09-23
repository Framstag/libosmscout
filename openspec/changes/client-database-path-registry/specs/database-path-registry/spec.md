# Spec Delta

## Purpose

Own the set of map database directories a client has been asked to open, register paths into it safely
from any thread, publish the complete set as a value so that other threads never read a list that is being
mutated, and make the number of database-set changes observable.

## ADDED Requirements

### Requirement: Registration is idempotent and keeps one entry per path

The registry SHALL hold every registered path exactly once, in registration order. Registering a path that
is already part of the set SHALL succeed without adding a second entry.

#### Scenario: Register a single path

- **WHEN** a path is registered
- **THEN** the set SHALL contain that path
- **AND** the operation SHALL report the path as part of the set

#### Scenario: Registering the same path twice keeps one entry

- **WHEN** the same path is registered twice
- **THEN** the set SHALL contain that path once
- **AND** the size of the set SHALL be one

#### Scenario: A one-element registration list equals a single registration

- **WHEN** a list of one path is registered and, separately, that path is registered on its own
- **THEN** both registries SHALL hold the same set

### Requirement: A registration publishes the complete set as a value snapshot

Every registration SHALL return the complete set as it is after the call, taken under the same lock that
protects the set, and the same complete value SHALL be available as a request. The returned value SHALL be
independent of the registry's own storage, so that a later registration cannot change or reallocate a
snapshot that a caller still holds. The lock SHALL NOT be held while a caller uses a snapshot.

#### Scenario: A snapshot is a value that later registrations do not change

- **GIVEN** a registry with one registered path
- **WHEN** a snapshot is taken and further paths are registered afterwards
- **THEN** the snapshot SHALL still report exactly the one path it was taken with
- **AND** a new snapshot SHALL report the complete, duplicate-free set

#### Scenario: The set is complete in every publication

- **GIVEN** a registry with an already registered set
- **WHEN** a list containing both new and already registered paths is registered
- **THEN** the published set SHALL contain the previously registered paths as well as the new ones
- **AND** no path SHALL appear twice

### Requirement: One registration call is one database-set change

The registry SHALL count one set change per registration call, independent of the number of paths in that
call. Registering a list SHALL report which inputs are part of the set afterwards and how many paths the
call added. Duplicates inside the list and paths already in the set SHALL NOT be counted as added.

#### Scenario: A list of paths is registered in one call

- **WHEN** a list of several paths is registered
- **THEN** the published set SHALL contain all of them
- **AND** the number of set changes SHALL be one
- **AND** the added count SHALL equal the number of paths that were not registered before

#### Scenario: The number of set changes does not depend on the list size

- **GIVEN** lists of 1, 8 and 64 paths
- **WHEN** each list is registered with one call, and the same paths are also registered one call at a time
- **THEN** the single batch call SHALL count one set change in each case
- **AND** the per-path loop SHALL count one set change per path

#### Scenario: Duplicates inside the list are skipped

- **GIVEN** a list that repeats a path and contains a path that is already registered
- **WHEN** the list is registered
- **THEN** the added count SHALL only cover the paths that were not part of the set before
- **AND** the set SHALL remain duplicate-free

### Requirement: The registry can be cleared and reports the set size

The registry SHALL report the number of registered paths and SHALL be clearable. Clearing SHALL forget
every registered path and SHALL keep counting registrations for the lifetime of the registry.

#### Scenario: Clear forgets the set but keeps counting set changes

- **GIVEN** a registry with registered paths and a recorded number of set changes
- **WHEN** the registry is cleared
- **THEN** the set SHALL be empty and report a size of zero
- **AND** the number of set changes SHALL be unchanged

### Requirement: Concurrent registrations leave a complete and consistent set

All operations SHALL be safe to call from any thread at any time. Registrations running concurrently SHALL
leave a set that contains every path of every successful call exactly once, and no registration SHALL fault
because another thread is mutating the set.

#### Scenario: Concurrent registration from several threads

- **GIVEN** a registry and several threads that each register a disjoint list of paths
- **WHEN** all registrations have completed
- **THEN** the set SHALL contain every path exactly once
- **AND** no snapshot taken during the run SHALL have shown a duplicate entry or a fault
