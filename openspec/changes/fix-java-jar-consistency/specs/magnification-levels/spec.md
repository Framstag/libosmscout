# Spec Delta

## Purpose

The magnification level is the index into the fixed table of map cell sizes; this capability defines the valid level range and keeps a lookup outside it from reading past that table.

## ADDED Requirements

### Requirement: A cell size lookup stays inside the cell dimension table

The magnification levels `0` up to the highest level the cell dimension table defines SHALL be the valid range for a level-based lookup of a map cell size, a tile's top-left coordinate, a tile's bounding box and the tile that contains a coordinate. A lookup with a level outside that range SHALL report an error and use the finest cell size the table defines instead of reading past the table, so that a release build (without asserts) and a build with asserts behave the same and neither reads out of bounds.

#### Scenario: A valid level resolves its own cell size

- **WHEN** a tile coordinate, bounding box or containing tile is computed with a level inside the valid range
- **THEN** the cell size of that level is used

#### Scenario: A level above the range is reported and clamped

- **GIVEN** a level above the highest level of the cell dimension table
- **WHEN** a cell size is looked up
- **THEN** an error naming the level and the supported range is reported
- **AND** the finest cell size of the table is used
- **AND** no memory outside the table is read

#### Scenario: The behaviour does not depend on asserts

- **GIVEN** two builds of the library that differ only in whether asserts are enabled
- **WHEN** the same out-of-range level is looked up in both
- **THEN** both report the level and use the finest cell size
- **AND** neither aborts and neither reads out of bounds
