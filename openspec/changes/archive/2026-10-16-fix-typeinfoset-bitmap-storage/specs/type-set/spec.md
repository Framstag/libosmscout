# Spec Delta

## Purpose

Defines what a set of types is for the database, map and import layers: which types it contains, the bytes
it allocates for them, the order in which iteration yields the contained types, and the result of the set
operations (union, difference, intersection, comparison).

## ADDED Requirements

### Requirement: The bytes a type set allocates follow the types it contains

The bytes a set of types allocates SHALL follow the types it contains and the number of types the type
configuration defines: at most one bit per defined type, the bitmap rounded up to a whole 64-bit word, plus
one slot per type the set contains. A set that contains few types of a type configuration with many defined
types SHALL NOT allocate a slot per
defined type.

#### Scenario: An empty set of a large type configuration allocates a bit per defined type

- **GIVEN** a type configuration with 1527 defined types
- **WHEN** a set of types is constructed for that type configuration and stays empty
- **THEN** the set allocates at most one bit per defined type

Test case: `TypeInfoSetStorageTest` — "An empty set of a 1527-type configuration allocates a bit per
defined type"

#### Scenario: A set of a few types of a large type configuration allocates per contained type

- **GIVEN** a type configuration with 1527 defined types and three of them
- **WHEN** a set of types is constructed from those three
- **THEN** it contains exactly those three types
- **AND** the set allocates a bitmap over the defined types plus a slot for each of the three, not a slot
  per defined type

Test case: `TypeInfoSetStorageTest` — "A set of a 1527-type configuration allocates per contained type, not
per defined type"

### Requirement: Iteration, membership and the operations of a type set are unchanged

A set of types SHALL contain exactly the types that were added to it and not removed from it, SHALL report
its size as the number of those types, SHALL yield them from iteration in ascending type index order, and
SHALL keep the results of union, difference, intersection, comparison and of clearing it as they are
today.

#### Scenario: Iteration yields the contained types in type index order

- **GIVEN** a set of types that contains several types of a type configuration, added in an order that
  differs from their type index order
- **WHEN** the set is iterated
- **THEN** iteration yields exactly the contained types, in ascending type index order

Test case: `TypeInfoSetStorageTest` — "Iteration of a set yields the contained types in type index order"

#### Scenario: The operations of a set agree with a reference model

- **GIVEN** a set of types and a reference model of a set of type indices
- **WHEN** types are added, removed, and a second set is unioned into, subtracted from, intersected with
  and compared against it
- **THEN** after each step the contained types, the size, the membership answers and the intersection and
  comparison results agree with the reference model

Test case: `TypeInfoSetStorageTest` — "The operations of a set agree with a reference model"

#### Scenario: A copy of a set keeps its own contents when the original changes

- **GIVEN** a set of types with several contained types and a copy of it
- **WHEN** the original is cleared and a different type is added to it
- **THEN** the copy still contains the types it had and compares unequal to the original

Test case: `TypeInfoSetStorageTest` — "A copy of a set keeps its own contents when the original changes"
