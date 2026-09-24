# fav-location-store Specification

## Purpose
Own a favorite location store, serialise every access to it, apply a wholesale replacement of its
content as one atomic step, report the "no store loaded" state instead of faulting, and destroy the
store explicitly.

## Requirements

### Requirement: The store owns the favorite service and serialises every access

The system SHALL provide a store component that owns the favorite location service instance and
serialises every access to it under a single lock. The service instance SHALL NOT be reachable by a
caller. Every operation of the component SHALL be safe to call from any thread at any time. The
component SHALL be non-copyable and SHALL destroy the service instance when it is destroyed.

#### Scenario: A call that runs during a replacement does not fault

- **GIVEN** a store that is being replaced repeatedly from one thread
- **WHEN** another thread keeps calling the mutating operations
- **THEN** no call SHALL fault
- **AND** the store SHALL end up in a complete generation of its content, not in a mixture of two

#### Scenario: The store cannot be copied

- **GIVEN** the store component's declaration
- **WHEN** a copy construction or a copy assignment is attempted
- **THEN** the program SHALL NOT compile

### Requirement: A wholesale replacement is never observed half applied

The store SHALL offer a replacement of its entire content as one operation, and that operation SHALL
run as one critical section, so that a concurrent reader observes the content either before or after
the replacement, never during it. This SHALL hold for a replacement with a store backed by a file and
for a replacement with caller-supplied content that is also persisted.

#### Scenario: A reader never sees a partially rebuilt store

- **GIVEN** a store with several groups of several favorites
- **WHEN** one thread replaces the content repeatedly while another thread reads the groups
- **THEN** every read SHALL return either no content or a complete generation
- **AND** no read SHALL return a generation with fewer groups or fewer favorites than that generation has

#### Scenario: The replacement with caller content is one step

- **GIVEN** caller-supplied content of several groups
- **WHEN** the content is handed to the store for replacement
- **THEN** the store SHALL contain that content in one step, not group by group

### Requirement: The store reports the no-store state instead of faulting

Before a store has been installed, and after it has been destroyed, the component SHALL report that no
store is loaded, the read operations SHALL return an empty result, and the mutating operations SHALL
report failure. Destroying the store SHALL be idempotent, SHALL be safe to call while another thread is
inside an operation, and SHALL leave the component usable: a later replacement SHALL install a store
again.

#### Scenario: A fresh store reports no store

- **GIVEN** a store that never had a file set
- **WHEN** the state is queried and a mutating operation is attempted
- **THEN** the component SHALL report that no store is loaded
- **AND** the read operations SHALL return empty results
- **AND** the mutating operations SHALL report failure

#### Scenario: A destroyed store reports no store and stays usable

- **GIVEN** a store with content
- **WHEN** the store is destroyed
- **THEN** the component SHALL report that no store is loaded
- **AND** the read operations SHALL return empty results
- **AND** the mutating operations SHALL report failure
- **AND** a second destruction SHALL succeed without effect
- **AND** a later replacement SHALL install content again

#### Scenario: An in-flight call cannot outlive the service instance

- **GIVEN** a store whose content is being replaced or destroyed
- **WHEN** another thread is inside a store operation
- **THEN** the operation SHALL complete against a live service instance
- **AND** the destruction SHALL wait for it rather than deleting the instance under it

### Requirement: The store forwards the favorite operations

The component SHALL expose the group and favorite operations (add, delete, rename, move, starred flag,
group color) and SHALL forward each of them to the service instance under the same lock, so a caller
never needs the service instance to perform a normal operation. The forwarded operation SHALL have the
same effect and the same result as calling the service directly, and SHALL report failure when no store
is loaded.

#### Scenario: Forwarded operations behave like the service operations

- **GIVEN** a store with content
- **WHEN** a group is added, a group is renamed, a favorite is added, renamed and moved, a favorite is
  starred and a group color is set through the component
- **THEN** each call SHALL have the effect the corresponding service operation has
- **AND** the read operations of the component SHALL report the resulting state

#### Scenario: Forwarded mutating operations fail without a store

- **GIVEN** a store with no loaded content
- **WHEN** any forwarding mutating operation is called
- **THEN** it SHALL report failure
- **AND** the read operations SHALL return empty results

### Requirement: A replacement keeps the supplied content when persisting it fails

When a replacement with caller-supplied content cannot be written to its file, the operation SHALL
report the failure and SHALL leave the supplied content in the store: the previous content SHALL NOT be
restored, and the in-memory state SHALL be the caller's snapshot.

#### Scenario: A failed write reports failure and keeps the supplied content

- **GIVEN** caller-supplied content and a file path that cannot be written
- **WHEN** the content is handed to the store for replacement
- **THEN** the operation SHALL report failure
- **AND** the store SHALL hold the supplied content in memory
- **AND** the store SHALL report that a store is loaded

#### Scenario: A successful replacement round-trips through the file

- **GIVEN** caller-supplied content with group attributes and favorites
- **WHEN** the content is handed to the store for replacement and the file is read back by a new
  service instance
- **THEN** the persisted content SHALL equal the supplied content

#### Scenario: A replacement discards the previous generation

- **GIVEN** a store with five groups
- **WHEN** it is replaced with content of one group
- **THEN** the store SHALL report exactly that one group
- **AND** the file SHALL hold exactly that one group
