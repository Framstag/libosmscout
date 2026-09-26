# Spec Delta

## Purpose

Gives starred favorite locations one user-chosen sequence that spans all groups, so a user can arrange their most important places in the order they want independent of which group each one belongs to.

## ADDED Requirements

### Requirement: Starred favorites have one order across all groups

The system SHALL provide a user-defined order over the starred favorites of the whole store. The order SHALL span groups: it SHALL NOT be derived from the group order, from the position of a favorite inside its group, or from any name. When two favorites are starred and the user arranges one before the other, that arrangement SHALL be what a reader observes and what is saved, so it survives a save and reload cycle.

Star membership SHALL keep its existing meaning: a favorite is starred exactly when it is marked as starred, and the order SHALL apply only to favorites that are starred. Reading the starred order SHALL return each starred favorite together with the group that holds it, so a caller can tell where each entry lives. A favorite that is not starred SHALL NOT appear in the starred order.

#### Scenario: The starred order is the order that was chosen

- **GIVEN** a store with starred favorites in two groups whose starred order is `Office,Home,Gym`
- **WHEN** the starred order is read
- **THEN** it SHALL be reported as `Office,Home,Gym`

#### Scenario: The starred order spans groups

- **GIVEN** a store with a starred favorite `Office` in the group `Work` and a starred favorite `Home` in the group `Home`
- **WHEN** the starred order is read
- **THEN** it SHALL contain both entries
- **AND** each entry SHALL name the group that holds it

#### Scenario: Only starred favorites appear

- **GIVEN** a group with the favorites `A`, `B` and `C` of which only `B` is starred
- **WHEN** the starred order is read
- **THEN** it SHALL contain exactly `B`

#### Scenario: Unstarring removes a favorite from the order

- **GIVEN** a starred order of `Office,Home,Gym`
- **WHEN** `Home` is unstarred
- **THEN** the starred order SHALL be reported as `Office,Gym`

#### Scenario: The starred order survives a save and reload cycle

- **GIVEN** a store whose starred order was arranged as `Gym,Office,Home` and then saved
- **WHEN** the store is reloaded from the same file by a new store instance
- **THEN** the starred order SHALL be `Gym,Office,Home`

#### Scenario: An empty store has an empty starred order

- **GIVEN** a store with no starred favorites
- **WHEN** the starred order is read
- **THEN** it SHALL be empty

### Requirement: A starred favorite can be moved within the starred order

The system SHALL provide an operation that moves an existing starred favorite to a target position in the starred order. The target position SHALL be a zero-based index into the starred order as that order looks after the favorite has been removed from its current position. A target position at or beyond the end of that order SHALL be clamped to the last position instead of failing. Moving a starred favorite to the position it already occupies SHALL succeed and SHALL leave the order unchanged.

A move SHALL change only the position of the moved favorite in the starred order: its name, coordinates and attributes SHALL be unchanged, its position inside its group and the order of the favorites of that group SHALL be unchanged, and the relative order of the other starred favorites SHALL be unchanged. An unknown group name, an unknown favorite name and a favorite that is not starred SHALL each be reported as a failure and SHALL leave the starred order unchanged. A move SHALL be serialized with the other write operations on the same store, so it SHALL never be observed half applied.

#### Scenario: Move a starred favorite to the front

- **GIVEN** a starred order of `Office,Home,Gym`
- **WHEN** `Gym` is moved to position `0`
- **THEN** the operation SHALL succeed
- **AND** the starred order SHALL be `Gym,Office,Home`

#### Scenario: The target position refers to the order after the removal

- **GIVEN** a starred order of `Office,Home,Gym`
- **WHEN** `Office` is moved to position `1`
- **THEN** the starred order SHALL be `Home,Office,Gym`

#### Scenario: A target position beyond the end is clamped

- **GIVEN** a starred order of `Home,Office,Gym`
- **WHEN** `Home` is moved to a position far beyond the end of the order
- **THEN** the operation SHALL succeed
- **AND** the starred order SHALL be `Office,Gym,Home`

#### Scenario: Moving a starred favorite to its current position

- **GIVEN** a starred order of `Office,Gym,Home`
- **WHEN** `Office` is moved to position `0`
- **THEN** the operation SHALL succeed
- **AND** the starred order SHALL remain `Office,Gym,Home`

#### Scenario: A starred favorite can be moved across groups

- **GIVEN** a starred order of `Office,Home,Gym` where `Office` and `Gym` are held by the group `Work` and `Home` by the group `Home`
- **WHEN** `Gym` is moved to position `0`
- **THEN** the starred order SHALL be `Gym,Office,Home`
- **AND** each entry SHALL still name the group that held it

#### Scenario: A move leaves the groups untouched

- **GIVEN** a group whose favorites are ordered `A,B,C` and a starred order of `C,A`
- **WHEN** `A` is moved to position `0` in the starred order
- **THEN** the starred order SHALL be `A,C`
- **AND** the group's favorites SHALL still be ordered `A,B,C`

#### Scenario: An unstarred favorite cannot be moved

- **GIVEN** a group with a favorite `D` that is not starred
- **WHEN** `D` is moved in the starred order
- **THEN** the operation SHALL fail
- **AND** the starred order SHALL be unchanged

#### Scenario: Unknown group or unknown favorite fails without changing anything

- **GIVEN** a starred order of `Office,Home,Gym`
- **WHEN** a move is attempted for a group that does not exist and for a favorite name that does not exist in an existing group
- **THEN** both operations SHALL fail
- **AND** the starred order SHALL remain `Office,Home,Gym`

#### Scenario: The moved starred order survives a save and reload cycle

- **GIVEN** a store whose starred order was arranged as `Gym,Home,Office` and then saved
- **WHEN** the store is reloaded from the same file by a new store instance
- **THEN** the starred order SHALL be `Gym,Home,Office`

### Requirement: Unstarring removes a star's position and starring appends at the end

Unstarring a favorite SHALL remove it from the starred order together with the position it had in that order. Starring a favorite SHALL place it at the end of the starred order. Starring a favorite that is already starred SHALL leave its position unchanged, and unstarring a favorite that is not starred SHALL leave the starred order unchanged.

The positions used by the starred order SHALL be owned by the store and SHALL NOT be part of the contract a caller relies on: a caller SHALL arrange stars by moving them, and SHALL NOT need to know, choose or maintain the values that hold the order.

#### Scenario: Starring appends at the end

- **GIVEN** a starred order of `Office,Home`
- **WHEN** the favorite `Gym` is starred
- **THEN** the starred order SHALL be `Office,Home,Gym`

#### Scenario: Starring again keeps the existing position

- **GIVEN** a starred order of `Office,Home,Gym`
- **WHEN** `Office` is starred again
- **THEN** the operation SHALL report success
- **AND** the starred order SHALL remain `Office,Home,Gym`

#### Scenario: Starring again after unstarring appends at the end

- **GIVEN** a starred order of `Office,Home,Gym`
- **WHEN** `Office` is unstarred and then starred again
- **THEN** the starred order SHALL be `Home,Gym,Office`

#### Scenario: Unstarring a favorite that is not starred changes nothing

- **GIVEN** a starred order of `Office,Home`
- **WHEN** an unstarred favorite is unstarred
- **THEN** the starred order SHALL remain `Office,Home`

#### Scenario: A starred position is not a caller-visible value

- **GIVEN** the favorites of a store
- **WHEN** the order of the starred favorites is arranged by moving them
- **THEN** no caller-visible operation SHALL require a caller to set or maintain a position value
- **AND** the arranged order SHALL be what a reader observes
