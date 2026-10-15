# Spec Delta

## MODIFIED Requirements

### Requirement: Service creates default fav locations file on first access
The system SHALL create an empty favorites file at the configured path when `FavoriteLocationService` is first constructed, if the file does not already exist. The file SHALL contain a document with the format version this change writes and an empty group list, so that every file the service creates is versioned.

#### Scenario: First access creates file
- **WHEN** `FavoriteLocationService` is constructed with a path that does not exist
- **THEN** the file SHALL be created
- **AND** the file SHALL carry the format version this change writes
- **AND** the file SHALL hold no groups
- **AND** loading it SHALL report no groups and SHALL be allowed to save over it

#### Scenario: Existing file is not overwritten
- **WHEN** `FavoriteLocationService` is constructed with a path to an existing file
- **THEN** the file content SHALL NOT be modified

## ADDED Requirements

### Requirement: A fav can be moved from one group to another

The system SHALL provide an operation that moves an existing fav out of its group and into another
group at a target position. The target position SHALL be a zero-based index into the destination
group's fav list as that list looks after the moved fav has been taken out of its own group. A target
position at or beyond the end of that list SHALL be clamped to the last position instead of failing,
so moving a fav to the front or to the end of the destination does not require the caller to know the
size of that group. Moving a fav into the group it already belongs to SHALL succeed and SHALL leave
the two positions unchanged.

The moved fav SHALL keep its data: its name, coordinates and all of its attributes, including a star
and the position of that star in the starred order, SHALL be exactly what they were before the move.
The source group's remaining favs SHALL keep their relative order, the destination group's favs SHALL
keep their relative order apart from the insertion, and no fav SHALL be duplicated or lost.

A fav name is unique inside a group. When the destination group already contains a fav of the name
being moved, the operation SHALL fail and SHALL leave both groups unchanged: the destination fav SHALL
NOT be replaced, removed or renamed. An unknown source group, an unknown destination group and an
unknown fav name SHALL each be reported as a failure and SHALL leave the stored state unchanged. A
move SHALL be serialized with the other write operations on the same store, so it SHALL never be
observed half applied.

#### Scenario: Move a fav from one group to another

- **GIVEN** a group `Home` whose fav order is `A,B,C` and a group `Work` whose fav order is `X,Y`
- **WHEN** `B` is moved from `Home` into `Work` at position `1`
- **THEN** the operation SHALL succeed
- **AND** `Home`'s fav order SHALL be `A,C`
- **AND** `Work`'s fav order SHALL be `X,B,Y`

#### Scenario: The target position refers to the destination list after the removal

- **GIVEN** a group `Home` whose fav order is `A,B,C` and a group `Work` whose fav order is `X,Y`
- **WHEN** `A` is moved from `Home` into `Work` at position `0`
- **THEN** `Work`'s fav order SHALL be `A,X,Y`

#### Scenario: Move a fav to the end of the destination group

- **GIVEN** a group `Home` whose fav order is `A,B,C` and a group `Work` whose fav order is `X,Y`
- **WHEN** `A` is moved from `Home` into `Work` at a position far beyond the end of `Work`
- **THEN** the operation SHALL succeed
- **AND** `Home`'s fav order SHALL be `B,C`
- **AND** `Work`'s fav order SHALL be `X,Y,A`

#### Scenario: Move into an empty group

- **GIVEN** a group `Home` whose fav order is `A` and an empty group `Work`
- **WHEN** `A` is moved from `Home` into `Work`
- **THEN** the operation SHALL succeed
- **AND** `Home` SHALL have no favs
- **AND** `Work`'s fav order SHALL be `A`

#### Scenario: The moved fav keeps its data

- **GIVEN** a fav that carries coordinates, attributes and a star, and that holds a position in the
  starred order
- **WHEN** the fav is moved into another group
- **THEN** its name, coordinates and attributes SHALL be unchanged
- **AND** it SHALL still be starred
- **AND** its position in the starred order SHALL be unchanged
- **AND** the group order SHALL be unchanged

#### Scenario: Move into the group the fav already belongs to

- **GIVEN** a group whose fav order is `A,B,C`
- **WHEN** a fav of that group is moved into that same group
- **THEN** the operation SHALL succeed
- **AND** the group's fav order SHALL be unchanged

#### Scenario: A name that already exists in the destination group refuses the move

- **GIVEN** a group `Home` whose fav order is `A,B,C` and a group `Work` whose fav order is `B,X`
- **WHEN** `B` is moved from `Home` into `Work`
- **THEN** the operation SHALL fail
- **AND** `Home`'s fav order SHALL remain `A,B,C`
- **AND** `Work`'s fav order SHALL remain `B,X`

#### Scenario: Unknown source group, destination group or fav fails without changing anything

- **GIVEN** a group `Home` whose fav order is `A,B,C` and a group `Work` whose fav order is `X,Y`
- **WHEN** a move is attempted out of a group that does not exist, into a group that does not exist,
  and for a fav name that `Home` does not contain
- **THEN** all three operations SHALL fail
- **AND** `Home`'s fav order SHALL remain `A,B,C`
- **AND** `Work`'s fav order SHALL remain `X,Y`

#### Scenario: The new group assignment survives a save and reload cycle

- **GIVEN** a store in which `B` was moved from `Home` into `Work`
- **WHEN** the store is saved and reloaded from the same file by a new store instance
- **THEN** `Home` SHALL no longer contain `B`
- **AND** `Work` SHALL contain `B` at the position it was given
