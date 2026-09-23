# Spec Delta

## ADDED Requirements

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
