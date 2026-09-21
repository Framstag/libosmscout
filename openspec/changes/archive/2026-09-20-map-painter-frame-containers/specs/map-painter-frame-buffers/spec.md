## Purpose

Defines the contract for the map painter's prepared per-frame data: the intermediate
representation of the areas, ways, way paths and route labels that a frame is built
from. It exists so that frame cost stays proportional to the visible object count
without per-frame allocation churn, while draw order, label placement and the read
access that backends have to prepared data all stay unchanged.

## ADDED Requirements

### Requirement: Prepared frame data is reused across frames

A rendered frame SHALL reuse the prepared per-frame data storage from the previous
frame instead of releasing and re-creating it, so that the number of heap
allocations per frame does not grow with the number of prepared objects.

#### Scenario: Repeated rendering of the same view does not allocate per object

- **GIVEN** a view whose prepared data contains areas, ways, way paths and route
  labels
- **WHEN** the same view is rendered repeatedly
- **THEN** the heap allocation count of the second and later frames SHALL NOT grow
  proportionally to the number of prepared objects

#### Scenario: Storage capacity survives a frame boundary

- **GIVEN** a painter that has rendered one frame for a view
- **WHEN** the next frame for the same view is prepared
- **THEN** the prepared storage SHALL still hold the capacity reached by the previous
  frame instead of being empty

### Requirement: Prepared frame data is contiguous per object kind

The prepared areas, ways and way paths of a frame SHALL each occupy one contiguous
block of storage, so that the draw steps traverse consecutive elements without a
per-object indirection.

#### Scenario: Consecutive prepared areas are adjacent

- **GIVEN** a frame whose prepared data contains at least two areas
- **WHEN** the prepared areas are inspected in draw order
- **THEN** each prepared area SHALL be located immediately after the previous one in
  its storage block

### Requirement: Draw order is stable for prepared areas and ways

The draw order of prepared areas and ways SHALL be identical to the order established
by the existing ordering criteria and SHALL be preserved for entries whose ordering
criteria compare equal.

#### Scenario: Equal-comparing areas keep their preparation order

- **GIVEN** two prepared areas with identical bounding boxes and the same outer/inner
  ring role
- **WHEN** the frame is prepared and the prepared areas are sorted
- **THEN** the two areas SHALL appear in the order in which they were prepared

#### Scenario: Repeated frames produce identical order

- **GIVEN** a view with several prepared areas and ways
- **WHEN** that view is rendered more than once
- **THEN** the draw order SHALL be identical in every frame

### Requirement: Route labels resolve their prepared way path

A prepared route label SHALL reference the prepared way path of the route segment it
belongs to, and that reference SHALL stay valid for the whole frame, including when
the prepared data storage grows or is reordered during preparation.

#### Scenario: Route label is placed on its own route segment

- **GIVEN** a route whose segments produce prepared way paths
- **WHEN** the route labels of the frame are prepared
- **THEN** each route label SHALL resolve to the prepared way path of its own route
  segment

#### Scenario: Reference stays valid after the prepared data grows

- **GIVEN** a prepared way path referenced by a route label
- **WHEN** further prepared data is added and the prepared data is sorted
- **THEN** the route label SHALL still resolve to the same prepared way path

### Requirement: Backends retain read access to prepared areas and ways

The post-preprocessing backend callback SHALL be able to enumerate and read all
prepared areas and ways of the frame, including their coordinate ranges.

#### Scenario: Backend callback reads all prepared areas and ways

- **GIVEN** a frame with prepared areas and ways
- **WHEN** the backend post-preprocessing callback enumerates the prepared areas and
  ways
- **THEN** it SHALL observe every prepared area and way of the frame together with
  their coordinate ranges

#### Scenario: Backend callback observes the draw order

- **GIVEN** a backend callback that enumerates the prepared areas and ways after
  postprocessing
- **WHEN** the frame is drawn
- **THEN** the enumeration order SHALL match the order in which the objects are drawn
