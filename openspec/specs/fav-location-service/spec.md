## Purpose

Persist and manage favorite location groups and favorites in a human-readable JSON file, with thread-safe read access and serialized writes.

## Requirements

### Requirement: Service creates default fav locations file on first access
The system SHALL create an empty JSON file at the configured path when `FavoriteLocationService` is first constructed, if the file does not already exist. The file SHALL contain an empty JSON object `{}`.

#### Scenario: First access creates file
- **WHEN** `FavoriteLocationService` is constructed with a path that does not exist
- **THEN** the file SHALL be created containing `{}`

#### Scenario: Existing file is not overwritten
- **WHEN** `FavoriteLocationService` is constructed with a path to an existing file
- **THEN** the file content SHALL NOT be modified

### Requirement: Service loads and saves groups with favs
The system SHALL persist a one-level grouping structure. Each group has a name (string). Each group contains zero or more favs. Each fav has a name (string) and a geographic coordinate (latitude, longitude). The data model SHALL use extensible attribute maps (`std::map<std::string, std::string>`) on both groups and favs to support future attributes without schema changes.

#### Scenario: Save and reload preserves all data
- **WHEN** a group with multiple favs is saved to file
- **THEN** reloading from the same file SHALL return identical groups and favs

#### Scenario: Empty group is persisted
- **WHEN** a group with no favs is saved
- **THEN** reloading SHALL return that group with an empty fav list

#### Scenario: Extensible attributes on groups
- **WHEN** a group has custom attributes set in its attribute map
- **THEN** those attributes SHALL be preserved across save/reload

#### Scenario: Extensible attributes on favs
- **WHEN** a fav has custom attributes set in its attribute map
- **THEN** those attributes SHALL be preserved across save/reload

### Requirement: Service supports CRUD for groups
The system SHALL provide methods to add, delete, and list groups. Adding a group with a name that already exists SHALL return an error or false. Deleting a non-existent group SHALL return an error or false.

#### Scenario: Add group succeeds
- **WHEN** a new group name is added
- **THEN** the group SHALL appear in the group list

#### Scenario: Add duplicate group fails
- **WHEN** a group name that already exists is added
- **THEN** the operation SHALL fail and the existing group SHALL remain unchanged

#### Scenario: Delete group succeeds
- **WHEN** an existing group is deleted
- **THEN** the group SHALL be removed from the group list

#### Scenario: Delete non-existent group fails
- **WHEN** a group name that does not exist is deleted
- **THEN** the operation SHALL fail

### Requirement: Service supports CRUD for favs within a group
The system SHALL provide methods to add, delete, and rename favs within a group. Adding a fav with a duplicate name within the same group SHALL fail. Deleting a non-existent fav SHALL fail. Renaming a fav to an existing name SHALL fail.

#### Scenario: Add fav to group succeeds
- **WHEN** a fav with a unique name is added to an existing group
- **THEN** the fav SHALL appear in that group's fav list

#### Scenario: Add fav with duplicate name fails
- **WHEN** a fav with a name that already exists in the group is added
- **THEN** the operation SHALL fail

#### Scenario: Add fav to non-existent group fails
- **WHEN** a fav is added to a group that does not exist
- **THEN** the operation SHALL fail

#### Scenario: Delete fav from group succeeds
- **WHEN** an existing fav is deleted from its group
- **THEN** the fav SHALL be removed from that group's fav list

#### Scenario: Rename fav succeeds
- **WHEN** an existing fav is renamed to a new unique name
- **THEN** the fav SHALL have the new name

#### Scenario: Rename fav to duplicate name fails
- **WHEN** an existing fav is renamed to a name already used in the same group
- **THEN** the operation SHALL fail

### Requirement: Service provides lookup by group and by name
The system SHALL provide methods to retrieve all favs in a group, and to find a specific fav by name within a group.

#### Scenario: Lookup favs by group
- **WHEN** a group name is queried
- **THEN** all favs in that group SHALL be returned

#### Scenario: Lookup non-existent group returns empty
- **WHEN** a non-existent group name is queried
- **THEN** an empty list SHALL be returned

#### Scenario: Lookup fav by name
- **WHEN** a group name and fav name are queried
- **THEN** the matching fav SHALL be returned

#### Scenario: Lookup non-existent fav returns null
- **WHEN** a non-existent fav name is queried within an existing group
- **THEN** null/empty SHALL be returned

### Requirement: JSON file format is human-readable
The persisted JSON file SHALL use indented formatting (2-space indent) for readability. The file SHALL use UTF-8 encoding.

#### Scenario: File is indented
- **WHEN** the file is written
- **THEN** it SHALL use 2-space indentation

#### Scenario: File is UTF-8
- **WHEN** the file is written
- **THEN** it SHALL be UTF-8 encoded

### Requirement: Service is thread-safe for read operations
Read operations (get groups, get favs, lookup by name) SHALL be safe to call from multiple threads concurrently. Write operations (add, delete, rename, save) SHALL be serialized.

#### Scenario: Concurrent reads do not block each other
- **WHEN** multiple threads call read methods simultaneously
- **THEN** all threads SHALL complete without data races

#### Scenario: Write blocks concurrent reads
- **WHEN** a write operation is in progress
- **THEN** concurrent read operations SHALL wait for the write to complete

### Requirement: A fav can be moved to another position within its group

The system SHALL provide an operation that moves an existing fav to a target position inside its
group. The target position SHALL be a zero-based index into the group's fav list as that list looks
after the fav has been removed from its current position. A target position at or beyond the end of
that list SHALL be clamped to the last position instead of failing, so moving a fav to the front or
to the end does not require the caller to know the size of the group. Moving a fav to the position
it already occupies SHALL succeed and SHALL leave the order unchanged.

The stored order SHALL be both the order a reader observes and the order that is saved, so a moved
fav SHALL keep its new position across a save and reload cycle. A move SHALL change only the
position of the moved fav: its name, coordinates and attributes SHALL be exactly what they were
before the move, and the relative order of all other favs of the group SHALL be unchanged. Groups
that the call does not name SHALL be unaffected, including groups that contain favs of the same
name.

An unknown group name and an unknown fav name SHALL both be reported as a failure, and SHALL leave
the stored state unchanged. A move SHALL be serialized with the other write operations on the same
store, so it SHALL never be observed half applied.

#### Scenario: Move a fav to the front

- **GIVEN** a group whose fav order is `A,B,C,D`
- **WHEN** `D` is moved to position `0`
- **THEN** the operation SHALL succeed
- **AND** the group's fav order SHALL be `D,A,B,C`

#### Scenario: The target position refers to the list after the removal

- **GIVEN** a group whose fav order is `A,B,C,D`
- **WHEN** `D` is moved to position `2`
- **THEN** the group's fav order SHALL be `A,B,D,C`
- **AND** only `D` SHALL have changed its position

#### Scenario: Move a fav to the last position

- **GIVEN** a group whose fav order is `A,B,D,C`
- **WHEN** `A` is moved to position `3`
- **THEN** the group's fav order SHALL be `B,D,C,A`

#### Scenario: A target position beyond the end is clamped

- **GIVEN** a group whose fav order is `B,D,C,A`
- **WHEN** `B` is moved to a position far beyond the end of the list
- **THEN** the operation SHALL succeed
- **AND** the group's fav order SHALL be `D,C,A,B`

#### Scenario: Moving a fav to its current position

- **GIVEN** a group whose fav order is `D,C,A,B`
- **WHEN** `D` is moved to position `0`
- **THEN** the operation SHALL succeed
- **AND** the group's fav order SHALL remain `D,C,A,B`

#### Scenario: A group with a single fav

- **GIVEN** a group that contains exactly one fav
- **WHEN** that fav is moved to its own position or to a position beyond the end
- **THEN** both operations SHALL succeed
- **AND** the group SHALL still contain that fav

#### Scenario: The moved fav keeps its data

- **GIVEN** a fav that carries coordinates and attributes
- **WHEN** the fav is moved to another position
- **THEN** its name, coordinates and attributes SHALL be unchanged
- **AND** no other fav of the group SHALL have lost or gained data

#### Scenario: Unknown group or unknown fav fails without changing anything

- **GIVEN** a store with a group `Work` whose fav order is `D,C,A,B`
- **WHEN** a move is attempted for a group that does not exist, and for a fav name that does not exist in `Work`
- **THEN** both operations SHALL fail
- **AND** the fav order of `Work` SHALL remain `D,C,A,B`

#### Scenario: A move affects only the named group

- **GIVEN** two groups that each contain favs named `Home,Work,Gym`
- **WHEN** `Home` is moved to the last position in the first group
- **THEN** the first group's order SHALL become `Work,Gym,Home`
- **AND** the second group's order SHALL remain `Home,Work,Gym`

#### Scenario: The moved order survives a save and reload cycle

- **GIVEN** a group whose favs were reordered to `Rome,Berlin,Paris` and then saved
- **WHEN** the store is reloaded from the same file by a new store instance
- **THEN** the group's fav order SHALL be `Rome,Berlin,Paris`
