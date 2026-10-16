# Spec Delta

## Purpose

Lets users choose and change the order in which favorite groups appear, so their own arrangement (for example home first, work last) is what they see and what is stored, instead of an order derived from the group names.

## ADDED Requirements

### Requirement: Groups have a user-defined order

The system SHALL report favorite groups in a user-defined order rather than in an order derived from their names. The order SHALL be the order a reader observes and the order that is saved, so it survives a save and reload cycle. A group that is added to a store SHALL be appended at the end of that order. Adding, deleting or renaming a group SHALL NOT reorder the remaining groups: deleting a group SHALL leave the relative order of the other groups unchanged, and renaming a group SHALL keep its position.

A group name SHALL remain unique, and lookup by name SHALL keep working regardless of the position the group has.

#### Scenario: The order a reader observes is the order that was chosen

- **GIVEN** a store with the groups `Work,Home,Uni` in that order
- **WHEN** the groups are read
- **THEN** they SHALL be reported as `Work,Home,Uni`

#### Scenario: A new group is appended at the end

- **GIVEN** a store with the groups `Work,Home,Uni` in that order
- **WHEN** the group `Gym` is added
- **THEN** the groups SHALL be reported as `Work,Home,Uni,Gym`

#### Scenario: Renaming a group keeps its position

- **GIVEN** a store with the groups `Work,Home,Uni` in that order
- **WHEN** `Home` is renamed to `Flat`
- **THEN** the groups SHALL be reported as `Work,Flat,Uni`
- **AND** the group SHALL still be found under the name `Flat`

#### Scenario: Deleting a group keeps the order of the others

- **GIVEN** a store with the groups `Work,Home,Uni` in that order
- **WHEN** `Home` is deleted
- **THEN** the groups SHALL be reported as `Work,Uni`

#### Scenario: The chosen order survives a save and reload cycle

- **GIVEN** a store whose group order was changed to `Work,Home,Uni`
- **WHEN** the store is saved and reloaded from the same file by a new store instance
- **THEN** the group order SHALL be `Work,Home,Uni`

#### Scenario: Two stores can hold the same groups in different orders

- **GIVEN** two stores with the same group names
- **WHEN** one store is given the order `Work,Home` and the other `Home,Work`
- **THEN** each store SHALL report its own order after a save and reload cycle

### Requirement: A group can be moved to another position

The system SHALL provide an operation that moves an existing group to a target position in the group order. The target position SHALL be a zero-based index into the group order as that order looks after the group has been removed from its current position. A target position at or beyond the end of that order SHALL be clamped to the last position instead of failing, so moving a group to the front or to the end does not require the caller to know how many groups there are. Moving a group to the position it already occupies SHALL succeed and SHALL leave the order unchanged.

A move SHALL change only the position of the moved group: its name, its attributes and its favorites, including their order, SHALL be exactly what they were before the move, and the relative order of all other groups SHALL be unchanged. An unknown group name SHALL be reported as a failure and SHALL leave the stored order unchanged. A move SHALL be serialized with the other write operations on the same store, so it SHALL never be observed half applied.

#### Scenario: Move a group to the front

- **GIVEN** a store with the groups `Work,Home,Uni`
- **WHEN** `Uni` is moved to position `0`
- **THEN** the operation SHALL succeed
- **AND** the groups SHALL be reported as `Uni,Work,Home`

#### Scenario: The target position refers to the order after the removal

- **GIVEN** a store with the groups `Uni,Work,Home`
- **WHEN** `Uni` is moved to position `1`
- **THEN** the groups SHALL be reported as `Work,Uni,Home`

#### Scenario: Move a group to the last position

- **GIVEN** a store with the groups `Work,Home,Uni`
- **WHEN** `Work` is moved to position `2`
- **THEN** the groups SHALL be reported as `Home,Uni,Work`

#### Scenario: A target position beyond the end is clamped

- **GIVEN** a store with the groups `Home,Uni,Work`
- **WHEN** `Home` is moved to a position far beyond the end of the order
- **THEN** the operation SHALL succeed
- **AND** the groups SHALL be reported as `Uni,Work,Home`

#### Scenario: Moving a group to its current position

- **GIVEN** a store with the groups `Home,Uni,Work`
- **WHEN** `Home` is moved to position `0`
- **THEN** the operation SHALL succeed
- **AND** the groups SHALL be reported as `Home,Uni,Work`

#### Scenario: A single group can be moved

- **GIVEN** a store that contains exactly one group
- **WHEN** that group is moved to its own position and to a position beyond the end
- **THEN** both operations SHALL succeed
- **AND** the store SHALL still contain that group

#### Scenario: A moved group keeps its content

- **GIVEN** a store with a group that carries a color attribute and several favorites in a known order
- **WHEN** the group is moved
- **THEN** its name, its attributes and its favorites in their order SHALL be unchanged
- **AND** no other group SHALL have lost or gained a favorite or an attribute

#### Scenario: Moving an unknown group fails without changing anything

- **GIVEN** a store with the groups `Work,Home,Uni`
- **WHEN** a group that does not exist is moved
- **THEN** the operation SHALL fail
- **AND** the group order SHALL remain `Work,Home,Uni`

### Requirement: Content written before this change is still read and is saved in the versioned, ordered form

A favorites file written before this change carries neither a format version nor a group order, and
holds its groups keyed by name. Loading such content SHALL succeed and SHALL NOT lose groups, favorites
or attributes. Because that form carries no order, loading SHALL report a defined order: the order those
groups have when sorted by name. Saving the store SHALL write the group order and the format version, so
a store loaded from such content and then saved SHALL report the same order on the next load.

Content that carries a group order SHALL be loaded with that order and SHALL be saved with it.

Reading the pre-version form is a compatibility path with a planned end: it exists so that an upgrade
does not lose favorites, and removing it is a follow-up concern rather than part of this change.

#### Scenario: Content written before this change loads with a defined order

- **GIVEN** stored content that holds the groups `Work`, `Home` and `Uni` in the pre-version form
- **WHEN** the content is loaded
- **THEN** the groups SHALL be reported as `Home,Uni,Work`
- **AND** every group SHALL still hold its favorites and attributes

#### Scenario: Pre-version content is written back in the versioned form

- **GIVEN** stored content in the pre-version form that was loaded and reported as `Home,Uni,Work`
- **WHEN** the store is saved and loaded again by a new store instance
- **THEN** the groups SHALL be reported as `Home,Uni,Work`
- **AND** the saved content SHALL carry the version this change writes

#### Scenario: Content with an order keeps that order

- **GIVEN** stored content that carries the group order `Work,Home,Uni`
- **WHEN** the content is loaded and saved again
- **THEN** the groups SHALL be reported as `Work,Home,Uni`

#### Scenario: Pre-version content with an empty group set is readable

- **GIVEN** stored content in the pre-version form that holds no groups
- **WHEN** the content is loaded
- **THEN** loading SHALL succeed
- **AND** the store SHALL report no groups

### Requirement: The persisted file carries a format version

Every saved favorites file SHALL carry the version of the format it was written in, so that a reader can
identify the content without inspecting its shape. Content without a version SHALL be treated as the
pre-version form described above. Content whose version is newer than the highest version the running
client knows SHALL NOT be read as a set of groups, and the file SHALL be protected: an operation that
would persist over that file SHALL fail instead of writing, so a client that does not understand the
newer content cannot destroy it. The state SHALL be readable, both as the version found in the file and
as whether that version is supported, so a caller can tell "this file was written by a newer client"
apart from "this file has no favorites".

Content whose version is known SHALL be read according to that version, and saving after such a read
SHALL be allowed.

#### Scenario: A saved file carries the version this change writes

- **WHEN** a store writes its content to a file and the file is inspected
- **THEN** the file SHALL carry the version this change writes

#### Scenario: A file without a version is treated as the pre-version form

- **GIVEN** a file that carries no version and holds groups keyed by name
- **WHEN** the file is loaded
- **THEN** the groups SHALL be read and reported
- **AND** the reported version for that file SHALL be the version of the pre-version form

#### Scenario: A file with a newer version is not read as groups

- **GIVEN** a file that carries a version newer than the highest version the client knows, and that holds groups
- **WHEN** the file is loaded
- **THEN** no groups SHALL be reported for it
- **AND** the state SHALL report the version found in the file
- **AND** the state SHALL report that this version is not supported

#### Scenario: A file with a newer version is not overwritten

- **GIVEN** a store that was opened on a file carrying a newer version, and that reports no groups
- **WHEN** the store is asked to save, and when it is asked to replace its content with caller-supplied content and persist it
- **THEN** both operations SHALL fail
- **AND** the file SHALL still hold the newer content unchanged

#### Scenario: A file with a supported version can be saved

- **GIVEN** a store opened on a file whose version the client knows
- **WHEN** the store content is changed and saved
- **THEN** the save SHALL succeed
- **AND** the file SHALL carry the version this change writes
