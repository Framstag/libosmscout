# Spec Delta

## Purpose

Defines what a renderer may rely on when it triangulates the polygon of an object: which input the
triangulation accepts, that a polygon it cannot handle is rejected instead of terminating the process,
and that the triangles of an accepted polygon do not change when the input repeats a point.

## ADDED Requirements

### Requirement: A polygon the triangulation cannot handle is rejected, not fatal

The triangulation SHALL either return the triangles of a polygon it accepts or yield no triangle for a
polygon it rejects. A rejection SHALL NOT terminate the process, SHALL NOT propagate as an exception to
the caller, and SHALL leave the caller able to continue.

#### Scenario: A ring of fewer than three distinct points is rejected

- **GIVEN** a ring of two points, and a ring whose points are two distinct values repeated
- **WHEN** the triangulation is asked for their triangles
- **THEN** each SHALL yield no triangle
- **THEN** it SHALL NOT terminate the process or throw at the caller

#### Scenario: A ring that collapses to a degenerate shape is rejected

- **GIVEN** a ring whose points are all collinear, so that no point of it is a corner
- **WHEN** the triangulation is asked for its triangles
- **THEN** it SHALL yield no triangle
- **THEN** the caller SHALL still be able to triangulate the next polygon

#### Scenario: The geometry captured from the OpenGL node path is rejected rather than fatal

- **GIVEN** the polygon that reaches the triangulation from the OpenGL backend's node path on the Dortmund database with `stylesheets/standard.oss`
- **WHEN** that polygon is triangulated
- **THEN** the triangulation SHALL return without terminating the process
- **THEN** the same input triangulated twice SHALL give the same result

### Requirement: A polygon is normalized before it is triangulated

The triangulation SHALL remove a point that repeats its predecessor and a closing point that repeats the
ring's first point before it triangulates, and SHALL require at least three distinct points. The triangles
of an accepted polygon SHALL NOT depend on the input repeating a point.

#### Scenario: A repeated closing point does not change the triangles

- **GIVEN** a square given as four points and the same square given as five points whose last point equals the first
- **WHEN** both are triangulated
- **THEN** the two results SHALL be equal

#### Scenario: A duplicated vertex does not change the triangles

- **GIVEN** a ring and the same ring with one of its vertices repeated
- **WHEN** both are triangulated
- **THEN** the two results SHALL be equal
- **THEN** the result SHALL cover the same area as the ring given without the repetition

### Requirement: The triangles of an accepted polygon are unchanged

For a polygon the triangulation accepts, the returned triangles SHALL be the triangles that were returned
before this change, so a renderer's output for that polygon does not change.

#### Scenario: The triangles of the shapes a renderer draws are unchanged

- **GIVEN** a triangle, a square, an L-shaped polygon and a polygon with a hole, each accepted by the triangulation
- **WHEN** they are triangulated
- **THEN** each result SHALL equal the result the same input produced before this change

### Requirement: A painter continues the frame and reports what it skipped

A painter that asks for the triangulation of a polygon that is rejected SHALL skip that polygon, SHALL
draw the rest of the frame, and SHALL report the skipped geometry so that the loss is visible. The
rejection SHALL NOT be reported as a failure of the frame.

#### Scenario: The frame's other objects are drawn after a rejected polygon

- **GIVEN** a frame whose geometry contains one polygon the triangulation rejects and other objects
- **WHEN** the frame is drawn
- **THEN** the other objects SHALL be drawn
- **THEN** the frame SHALL complete and the run SHALL NOT terminate

#### Scenario: The skipped geometry is named in the report

- **GIVEN** a painter that skipped a polygon because the triangulation rejected it
- **WHEN** the run's log is read
- **THEN** it SHALL name the object that was skipped and the reason the triangulation gave

#### Scenario: The shipped area styles render on the Dortmund database again

- **GIVEN** the OpenGL performance run over `maps/Dortmund` with `stylesheets/standard.oss`, `cycle.oss` and `winter-sports.oss`
- **WHEN** the run completes
- **THEN** it SHALL finish without a crash
- **THEN** the areas of those stylesheets SHALL have been drawn rather than skipped wholesale
