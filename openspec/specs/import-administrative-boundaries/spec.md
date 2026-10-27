# import-administrative-boundaries Specification

## Purpose
Defines what the import pipeline treats as an administrative boundary and what a boundary becomes in the
location index, so that a boundary type the type definition declares is never silently dropped by a step
that enumerates boundary types on its own.

## Requirements

### Requirement: Every declared boundary type is treated as a boundary

The import pipeline SHALL treat every type of the loaded type definition that the pipeline resolves as an
administrative boundary as a boundary, for the location index and for the composition of a boundary
relation alike. A boundary of an `admin_level` the import evaluates SHALL become a region of the location
index, regardless of which of the declared boundary types it resolves to.

#### Scenario: A level-8 boundary becomes a region

- **GIVEN** a type definition that declares a boundary type for `admin_level=8`
- **AND** OSM data that carries a boundary area with `admin_level=8`
- **WHEN** the data is imported
- **THEN** the location index SHALL carry a region for that boundary
- **AND** the region's parent SHALL be the region of the enclosing county

Test case: `LocationBoundaryRegionTest` — "A level-8 boundary is a region of the location index"

#### Scenario: A level-10 boundary nested in a level-8 boundary becomes a nested region

- **GIVEN** a type definition that declares a boundary type for `admin_level=10`
- **AND** OSM data that carries a boundary area with `admin_level=10` inside a boundary area with
  `admin_level=8`
- **WHEN** the data is imported
- **THEN** the location index SHALL carry a region for the level-10 boundary
- **AND** its parent SHALL be the region of the level-8 boundary

Test case: `LocationBoundaryRegionTest` — "A level-10 boundary is a region of the location index, nested in the municipality"

#### Scenario: The shipped type definition resolves every boundary type it declares

- **GIVEN** the shipped `stylesheets/map.ost`, which declares `boundary_country`, `boundary_state`,
  `boundary_county`, `boundary_municipality`, `boundary_suburb` and `boundary_administrative`
- **WHEN** the pipeline resolves the types it treats as administrative boundaries
- **THEN** the resolved set SHALL contain each of those types

Test case: `BoundaryTypesTest` — "The shipped type definition resolves every boundary type it declares"

### Requirement: A type definition without the newer boundary types still imports

A type definition that does not declare `boundary_municipality` or `boundary_suburb` SHALL import without
aborting the run. The pipeline SHALL resolve those names only when the type definition declares them, and
SHALL report the absence so that a boundary that is not a region is not a silently dropped one.

#### Scenario: A legacy type definition resolves without the two types

- **GIVEN** a type definition that declares only `boundary_country`, `boundary_state`, `boundary_county`
  and `boundary_administrative`
- **WHEN** the pipeline resolves the types it treats as administrative boundaries
- **THEN** the resolved set SHALL contain the four declared types
- **AND** it SHALL contain neither `boundary_municipality` nor `boundary_suburb`
- **AND** no assertion SHALL fire

Test case: `BoundaryTypesTest` — "A type definition without the municipality and suburb types resolves without them"

### Requirement: Both steps that read boundary types agree

The location index step and the relation-to-area step SHALL treat the same set of types as administrative
boundaries, so that a boundary can neither become a region without being composed as a boundary relation
nor the other way round.

#### Scenario: The two steps resolve the same set

- **GIVEN** any type definition
- **WHEN** the location index step and the relation-to-area step each resolve the boundary types
- **THEN** both resolve the same set of types

Test case: `BoundaryTypesTest` — "The resolver is the single source of the boundary type set"; covered by
construction, as both steps call the one resolver.
