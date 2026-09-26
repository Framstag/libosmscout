# Spec Delta

## MODIFIED Requirements

### Requirement: The store forwards the favorite operations

The component SHALL expose the group and favorite operations (add, delete, rename, move, group
positioning, moving a favorite to another group, starred flag, starring order, group color) and SHALL
forward each of them to the service instance under the same lock, so a caller
never needs the service instance to perform a normal operation. The forwarded operation SHALL have the
same effect and the same result as calling the service directly, and SHALL report failure when no store
is loaded. The group reader and the starred-order reader SHALL likewise be forwarded and SHALL return
an empty result when no store is loaded.

#### Scenario: Forwarded operations behave like the service operations

- **GIVEN** a store with content
- **WHEN** a group is added, a group is renamed, a favorite is added, renamed and moved, a favorite is
  starred and a group color is set through the component
- **THEN** each call SHALL have the effect the corresponding service operation has
- **AND** the read operations of the component SHALL report the resulting state

#### Scenario: The positioning operations behave like the service operations

- **GIVEN** a store with several groups, favorites and starred favorites
- **WHEN** a group is moved, a favorite is moved into another group, and a starred favorite is moved
  within the starred order through the component
- **THEN** each call SHALL have the effect the corresponding service operation has
- **AND** the group reader and the starred-order reader of the component SHALL report the resulting
  order

#### Scenario: A refused operation reports failure through the component

- **GIVEN** a store in which the destination group of a favorite move already holds a favorite of that
  name
- **WHEN** the move is attempted through the component
- **THEN** it SHALL report failure
- **AND** the read operations SHALL report the state of both groups unchanged

#### Scenario: Forwarded mutating operations fail without a store

- **GIVEN** a store with no loaded content
- **WHEN** any forwarding mutating operation is called
- **THEN** it SHALL report failure
- **AND** the read operations SHALL return empty results

## ADDED Requirements

### Requirement: A replacement preserves the group order of the supplied content

When the store replaces its content with caller-supplied content, it SHALL keep the order of the groups
it was given: the order SHALL be what a reader of the store observes and SHALL be the order persisted to
the file. Replacing the content SHALL NOT reorder the groups, and a replacement followed by a reload from
the file SHALL report the same group order and the same starred order that were supplied.

#### Scenario: A replacement keeps the supplied group order

- **GIVEN** caller-supplied content whose groups are ordered `Work,Home,Uni`
- **WHEN** the content is handed to the store for replacement
- **THEN** the store's group reader SHALL report `Work,Home,Uni`

#### Scenario: The supplied order round-trips through the file

- **GIVEN** caller-supplied content whose groups are ordered `Work,Home,Uni` and whose starred favorites
  are ordered `Office,Gym`
- **WHEN** the content is handed to the store for replacement and the file is read back by a new service
  instance
- **THEN** the group order SHALL be `Work,Home,Uni`
- **AND** the starred order SHALL be `Office,Gym`

#### Scenario: Repeated replacements with different orders do not mix them

- **GIVEN** a store that is repeatedly replaced with content ordered `Uni,Work,Home` and then with
  content ordered `Work,Home,Uni`
- **WHEN** a reader observes the groups during the replacements
- **THEN** every read SHALL report one of the two complete orders
- **AND** no read SHALL report a mixture of the two

### Requirement: The store reports the file version and refuses to persist over an unsupported one

The store SHALL report the format version of the file it was opened on and whether the client supports
that version. When the file carries a version newer than the client knows, the store SHALL report that
version as unsupported, SHALL report no groups, and SHALL fail every operation that would persist over
that path — both saving the current content and replacing the content with caller-supplied content and
persisting it — leaving the file unchanged. The in-memory effect of a refused replacement SHALL follow
the documented behaviour of a failed write. When the file's version is supported, the state SHALL report
it as supported and persisting SHALL be allowed. Before a store is installed, the state SHALL report
that no version is known.

#### Scenario: An unsupported version is reported and its groups are not read

- **GIVEN** a store opened on a file that carries a version newer than the client knows and that holds groups
- **WHEN** the state is queried
- **THEN** it SHALL report the version found in the file
- **AND** it SHALL report that version as unsupported
- **AND** the group reader SHALL report no groups

#### Scenario: An unsupported file is never overwritten

- **GIVEN** a store opened on a file that carries a newer version
- **WHEN** saving is attempted, and when a replacement with caller-supplied content that persists is attempted
- **THEN** both operations SHALL report failure
- **AND** the file SHALL still hold the newer content unchanged

#### Scenario: A supported version can be persisted

- **GIVEN** a store opened on a file whose version the client knows, and a store opened on a file in the pre-version form
- **WHEN** content is added and saved in each
- **THEN** both saves SHALL succeed
- **AND** both files SHALL carry the version this change writes

#### Scenario: A store that is not installed reports no version

- **GIVEN** a store with no loaded content
- **WHEN** the version state is queried
- **THEN** it SHALL report that no version is known
- **AND** a save SHALL still report failure
