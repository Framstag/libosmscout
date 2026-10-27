# Spec Delta

## Purpose

Defines what a set of types is for the database, map and import layers: which types it contains, the bytes
it allocates for them, the order in which iteration yields the contained types, and the result of the set
operations (union, difference, intersection, comparison).

## MODIFIED Requirements

### Requirement: The bytes a type set allocates follow the types it contains

The bytes a set of types allocates SHALL follow the types it contains and the number of types the type
configuration defines: at most one bit per defined type, the bitmap rounded up to a whole 64-bit word, plus
one slot per element of the allocated capacity of the contained-type vector. A set that contains few types
of a type configuration with many defined types SHALL NOT allocate a slot per
defined type. A set SHALL report those bytes.

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

#### Scenario: An empty set reports the bitmap bytes it allocates

- **GIVEN** an empty set of types, of a type configuration with 1527 defined types, for which a
  construction allocated storage
- **WHEN** the set is asked for the bytes it allocates
- **THEN** the answer is the number of bytes that construction allocated for the set's bitmap, one bit per
  defined type rounded up to a whole 64-bit word

Test case: `TypeInfoSetStorageTest` — "A set reports the bytes it allocates"

The accessor reports the allocated storage, so the contained-type term counts the vector's `capacity()`,
not its `size()`. That term is not asserted by a case: the case's set is empty, so it never compares that
term against the allocation counter.
