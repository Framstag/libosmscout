# Spec Delta

## Purpose

Defines the contract of the geometry optimizer that turns a projected way or area into the geometry a
painter draws or an importer stores: the output is simple and closed, and it does not depend on the
storage state of the sequence the optimizer works on.

## ADDED Requirements

### Requirement: Optimized geometry is simple and keeps the points it was given

The optimizer SHALL produce a geometry that does not intersect itself, treating an area ring as
closed: the segment from the ring's last point back to its first point SHALL be part of the
simplicity decision. A way SHALL be treated as an open line. The optimizer SHALL NOT drop a point of
the input beyond the reduction the optimization is asked for, and it SHALL NOT add a duplicate
closing coordinate to the drawn geometry, because the consumer that draws an area closes the ring
itself.

#### Scenario: An optimized way does not intersect itself

- **GIVEN** a projected way whose geometry intersects itself
- **WHEN** the way is optimized for drawing
- **THEN** the resulting geometry SHALL contain no intersection between its segments
- **AND** the resulting geometry SHALL keep the start and the end point of the input

#### Scenario: An optimized area ring does not intersect itself when it is closed

- **GIVEN** a projected area ring whose geometry intersects itself once the segment from its last point back to its first point is part of it
- **WHEN** the ring is optimized for drawing
- **THEN** the resulting geometry SHALL contain no intersection between its segments when it is read as a closed ring
- **AND** the resulting geometry SHALL contain no more points than the ring it was given

### Requirement: Optimization is independent of the storage state of the optimized sequence

The optimizer SHALL produce the same geometry whether or not the storage of the sequence it optimizes
has to grow while it works, in particular when it appends a point of that same sequence. Reading a
point for such an append SHALL NOT depend on an element the growing sequence has left behind.

#### Scenario: A sequence that has to grow keeps the geometry

- **GIVEN** a ring whose number of drawn points is exactly the size of the storage of the optimized sequence when it appends the point that closes the ring for the simplicity decision, so that the append has to grow that storage
- **WHEN** the ring is optimized for drawing
- **THEN** the resulting geometry SHALL keep every point of the ring
- **AND** the ring SHALL be simple when it is read as a closed ring

#### Scenario: A sequence with spare storage keeps the geometry

- **GIVEN** a ring of the same shape that is optimized with spare storage for the point that closes it for the simplicity decision
- **WHEN** the ring is optimized for drawing
- **THEN** the resulting geometry SHALL keep every point of the ring
- **AND** the ring SHALL be simple when it is read as a closed ring

### Requirement: The optimizer compiles without a possible-uninitialized-use diagnostic

A Release build of the library SHALL report no possible-uninitialized-use diagnostic for the
optimizer, so that the build's warning gate covers the appends the optimizer performs.

#### Scenario: A Release build of the optimizer is warning free

- **GIVEN** the library's Release build with the warning flags of the project's build configurations
- **WHEN** the translation unit that holds the optimizer is compiled
- **THEN** the compiler SHALL report no possible-uninitialized-use diagnostic for it
- **AND** no diagnostic that the appends of the optimizer produce SHALL be suppressed to reach that state
