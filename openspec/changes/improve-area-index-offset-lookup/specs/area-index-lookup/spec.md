# Spec Delta

## Purpose

Defines what a lookup on the area index of a database costs relative to the request it serves and
what its result is, so that a lookup can be answered at the cost of the types the request names
rather than at the cost of the types the index holds.

## ADDED Requirements

### Requirement: An area-index lookup examines the types its request names

The work an area-index lookup performs SHALL follow the types the request names, not the types the
index holds. A request SHALL NOT cause the lookup to examine an index entry of a type the request
does not name.

#### Scenario: A request that names a subset touches only that subset

- **GIVEN** an area index that carries index entries for more area types than a request names
- **WHEN** the lookup serves that request
- **THEN** it SHALL examine the index entries of the types the request names
- **AND** it SHALL NOT examine an index entry of a type the request does not name

#### Scenario: Adding indexed types outside the request does not enlarge the lookup

- **GIVEN** two indexes that differ only in the number of area types they carry entries for, all of
  those types outside one request
- **WHEN** that request is served against both
- **THEN** the lookup SHALL examine the same entries in both
- **AND** that set SHALL be the entries of the types the request names

### Requirement: An area-index lookup reports the request's types as loaded

A lookup SHALL report a type as loaded when the request names it and the index carries an entry for
it, whether or not that entry resolves an offset for the queried box. A lookup SHALL NOT report a
type the request does not name.

#### Scenario: Every named type the index carries appears as loaded

- **GIVEN** an index that carries an entry for a type a request names
- **WHEN** the lookup serves that request
- **THEN** that type SHALL be reported as loaded
- **AND** a type the request does not name SHALL NOT be reported as loaded

#### Scenario: A named type that resolves no offset is still reported as loaded

- **GIVEN** a request whose named type has an index entry that resolves no offset for the queried box
- **WHEN** the lookup serves the request
- **THEN** that type SHALL still be reported as loaded

### Requirement: The result of an area-index lookup follows the request, not the type set

For one request, a lookup on an index SHALL resolve the same offsets and report the same loaded types
as the same request on an index that differs only in the types the request does not name. A larger
area type set SHALL NOT change the result of a request.

#### Scenario: A request resolves no offset of a type it does not name

- **GIVEN** a requested type and an unnamed type whose index entries both intersect the queried box
- **WHEN** the lookup serves the request
- **THEN** the resolved offsets SHALL include the offsets of the requested type
- **AND** they SHALL include no offset of the unnamed type

#### Scenario: A larger type set leaves the result of a request unchanged

- **GIVEN** two indexes that differ only in the entries of types a request does not name
- **WHEN** the same request is served against both
- **THEN** the offsets the lookup resolves SHALL be the same in both
- **AND** the types it reports as loaded SHALL be the same in both
